#pragma once
#include <vector>
#include <string>
#include <gui.h>
#include "structs.h"
#include "json.h"
#include "noise_generator.h"

class PlotManager {
private:
	enum function_names {
		sin,
		cos,
		saw,
		sqare,
		random,
		math_expression
	};

	struct Signal {
		//INPUT
		std::string input_name;
		ImVec4 plot_color;
		PlotManager::function_names type;
		std::string math_expr;
		float amplitude;
		ScrollingBuffer data_buffer_analog;
		float noise;

		//Output digital
		ScrollingBuffer data_buffer_sampling;
		float last_sample_time;
		ScrollingBuffer data_buffer_quantization;
		ScrollingBuffer data_buffer_quantization_index;
		ScrollingBuffer data_buffer_quantization_binary;
		NoiseGenerator noise_gen;

		Signal(std::string name, PlotManager::function_names math_func, 
			std::string math_expression, ImVec4 color, float amplitude_, float noise_)
			: input_name(name), type(math_func), math_expr(math_expression), 
			plot_color(color), amplitude(amplitude_), last_sample_time(0.f), noise(noise_) {};

		Signal()
			: input_name("name"), type(function_names::sin), math_expr("math_express"),
			plot_color({1.f, 0.5f, 0.1f, 1.f}), amplitude(1.f), last_sample_time(0.f), noise(0.f) {};
	};

public:
	PlotManager();
	~PlotManager() {};

	//Plot data
	void TickData();

	//GUI
	void OpenAddInputDialog();
	void RenderAddInputDialog();
	void RenderInputCards();
	void RenderMainPlot();
	void RenderDigitizationOtions();
	void RenderMainPlotSettings();
	void RenderTextOutput();

	void ClearAllInputs() { inputs.clear(); };
	void LoadJsonData(nlohmann::json& data);
	nlohmann::json GetJsonData();
 private:
	//GUI
	void OpenEditInputDialog();
	void RenderEditInputDialog();
	int RenderAnalogInputCard(Signal& input);
	void PlotDataTooltip(const char* name, const float* xs, const float* value, float width_percent, int count, int stride);
	std::vector<std::string> vec_function_names;
	float marker_size = 3.2f;
	bool auto_size = true;
	float DigitalBitHeight = 25.f;

	void ExportDataToFile(int data_input_index, std::string filePathName, bool use_dot, int data_format);

	//Digital
	float FindClosestQuantValue(float value, float quant_step, int number_of_positions, float min, float max);
	int FindClosestQuantIndex(float value, float quant_step, int number_of_positions, float min, float max);
	void TickOutputData(Signal& output);
	float sampling_rate = 1.5f;
	bool show_sampling = false;
	bool add_noise = false;
	bool sync_sample_timing = false;
	float last_sampled_time_global = 0.f;
	float noise_multiplier = 1.f;
	int sample_show_type = 0;

	//quantizace
	void GenerateQuantiziedValue(Signal& output, float last_y_axis_value, float qunat_step, int number_of_positions);
	void GenerateQuantiziedIndex(Signal& output, float last_y_axis_value, float qunat_step, int number_of_positions);
	void GenerateQuantiziedBinary(Signal& output, float time_delta);
	double max_quant_value = 1.f;
	double min_quant_value = -1.f;
	bool show_quant_limits = false;
	int quant_bit_depth = 4;
	bool show_quant_data = false;
	bool show_quant_levels = false;
	int quant_show_type = 0;

	bool show_digital_data = false;
	bool show_digital_data_text = false;
	int digital_data_type = 0; //bits, hex, decimal
	std::string NumberToBitString(float number);
	std::string NumberToHexString(float number);

	//Input
	bool CheckIfInputNameExists(std::string name, int skip = -1);
	float ProcessMathExpression(Signal& input);
	void TickInputData(Signal& input);
	void DeleteInput(size_t index);
	std::vector<Signal> inputs;
	int edit_input = 0;
	bool paused = false;
	float time_scale = 1.f;
	float time_ammount = 10.f;

	float time = 0.f;
};

extern PlotManager plot_manager;
#pragma once
#include <vector>
#include <string>
#include <gui.h>
#include "structs.h"

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

		//Output digital
		ScrollingBuffer data_buffer_sampling;
		float last_sample_time;
		ScrollingBuffer data_buffer_quantization;
		
		Signal(std::string name, PlotManager::function_names math_func, 
			std::string math_expression, ImVec4 color, float amplitude_)
			: input_name(name), type(math_func), math_expr(math_expression), 
			plot_color(color), amplitude(amplitude_), last_sample_time(0.f) {};
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
	void RenderDigitalizationOtions();
	void RenderMainPlotSettings();
private:
	//GUI
	void OpenEditInputDialog();
	void RenderEditInputDialog();
	int RenderAnalogInputCard(Signal& input);
	std::vector<std::string> vec_function_names;
	float marker_size = 3.2f;
	bool auto_size = true;

	//Digital
	float GenerateGussianNoise();
	float FindClosestQuantValue(float value, float quant_step, int number_of_positions, float min, float max);
	void TickOutputData(Signal& output);
	float sampling_rate = 1.5f;
	bool show_sampling = false;
	bool add_noise = false;
	float noise_multiplier = 1.f;
	int sample_show_type = 0;

	//quantizace
	double max_quant_value = 1.f;
	double min_quant_value = -1.f;
	bool show_quant_limits = false;
	int quant_bit_depth = 4;
	bool show_quant_data = false;
	bool show_quant_levels = false;
	int quant_show_type = 0;

	//Input
	bool CheckIfInputNameExists(std::string name, int skip = -1);
	void ProcessMathExpression(Signal& input);
	void TickInputData(Signal& input);
	void DeleteInput(size_t index);
	std::vector<Signal> inputs;
	int edit_input = 0;
	bool paused = false;

	float time = 0.f;
};

extern PlotManager plot_manager;
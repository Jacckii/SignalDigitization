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

	struct AnalogInput {
		std::string input_name;
		ImVec4 plot_color;
		PlotManager::function_names type;
		std::string math_expr;
		float amplitude;
		ScrollingBuffer data_buffer;
		AnalogInput(std::string name, PlotManager::function_names math_func, 
			std::string math_expression, ImVec4 color, float amplitude_)
			: input_name(name), type(math_func), math_expr(math_expression), 
			plot_color(color), amplitude(amplitude_) {};
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
private:
	//GUI
	void OpenEditInputDialog();
	void RenderEditInputDialog();
	int RenderAnalogInputCard(AnalogInput& input);
	std::vector<std::string> vec_function_names;

	//Digital
	float GenerateGussianNoise();
	int sampling_rate = 5;

	//Input
	bool CheckIfInputNameExists(std::string name, int skip = -1);
	void ProcessMathExpression(AnalogInput& input);
	void TickInputData(AnalogInput& input);
	void DeleteInput(size_t index);
	std::vector<AnalogInput> inputs;
	int edit_input = 0;

	float time = 0.f;
};

extern PlotManager plot_manager;
#include <exprtk/exprtk.hpp>
#include "plot_manager.h"
#include <optional>
#include <MagicEnum/magic_enum.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <implot.h>
#include <random>
#include <implot_internal.h>
#include <ImGuiFileDialog.h>

#include <iostream>
#include <fstream>
#include <vector>
#include "noise_generator.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#endif

PlotManager plot_manager;

typedef exprtk::symbol_table<float> symbol_table_t;
typedef exprtk::expression<float> expression_t;
typedef exprtk::parser<float>         parser_t;

PlotManager::PlotManager()
{
    constexpr auto enum_data = magic_enum::enum_values<PlotManager::function_names>();
    for (auto& iter : enum_data) {
        vec_function_names.push_back(std::string(magic_enum::enum_name(iter)));
    }
}

void PlotManager::TickData()
{
    if (paused)
        return;

    time += ImGui::GetIO().DeltaTime * time_scale;

    for (auto& iter : inputs) {
        TickInputData(iter);
        TickOutputData(iter);
        iter.noise_gen.tick(time);
    }

    auto sampling_rate_in_ms = 1.f / (float)sampling_rate;
    if (last_sampled_time_global + sampling_rate_in_ms < time) {
        //sampling
        last_sampled_time_global = time;
    }
}

void PlotManager::OpenAddInputDialog()
{
    ImGui::OpenPopup("Add input");
}

void PlotManager::RenderAddInputDialog()
{
    if (ImGui::BeginPopupModal("Add input", NULL))
    {
        static int uid = 0;
        static char name[256] = "Analog_0\0";
        static char math_expression[500] = "\0";
        static int item = 0;
        static float amplitude = 1.f;
        static float noise = 0.f;
        ImGui::Text("Select input type:");
        ImGui::ComboV("##input_function", &item, vec_function_names);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select how should we generate the signal");
        ImGui::Text("Input name:");
        ImGui::InputText("##InputNameInput", name, 256);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Name of the input signal, you will be able to see this in legend of the plot");
        static float color[4] = { 0.4f, 0.7f, 0.0f, 1.f };
        ImGui::Text("Plot color:");
        ImGui::ColorEdit4("##PlotColor", color);

        if (item == PlotManager::function_names::math_expression) {
            ImGui::Text("Math expression:");
            ImGui::InputText("##MathExpressionInput", math_expression, 500);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Type here an mathematic expression, you can use variable x to define time. For ex. 'sin(x)'");
        }
        else {
            ImGui::Text("Amplitude:");
            ImGui::SliderFloat("##AmplitudeSlider", &amplitude, 0.01f, 1.f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("This allows you to multiply the amplitude of given function");
        }

        ImGui::Text("Noise:");
        ImGui::SliderFloat("##noiseAddInput", &noise, 0.00f, 5.f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("You can add noise to the signal to make it more realistic");

        ImGui::Dummy(ImVec2(300.f, 15.f));

        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.31f, 0.98f, 0.48f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.31f, 0.98f, 0.48f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.41f, 0.99f, 0.57f, 0.7f));
        if (ImGui::Button("Add"))
        {
            auto new_name = std::string(name);
            if (CheckIfInputNameExists(new_name))
                new_name += std::string("#") + std::to_string(uid);

            PlotManager::Signal temp(
                std::string(new_name),
                (PlotManager::function_names)item,
                std::string(math_expression),
                ImVec4(color[0], color[1], color[2], color[3]),
                amplitude, noise);

            inputs.push_back(temp);
            uid++;
            sprintf(name, "Analog_%d\0", uid);

            //generate new color using colormaps
            ImPlot::BeginPlot("##DUMMY");
            auto new_color = ImPlot::NextColormapColor();
            ImPlot::EndPlot();

            color[0] = new_color.x;
            color[1] = new_color.y;
            color[2] = new_color.z;
            color[3] = new_color.w;

            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
}

void PlotManager::RenderInputCards()
{
    for (size_t i = 0; i < inputs.size(); i++) {
        auto& iter = inputs[i];
        auto ret = RenderAnalogInputCard(iter);
        if (ret == 0)
            continue;

        //edit
        if (ret == 1)
        {
            edit_input = (int)i;
            OpenEditInputDialog();
        }

        if (ret == 2) {
            edit_input = -1;
            DeleteInput(i);
            break;
        }
    }
    RenderEditInputDialog();
}

void PlotManager::RenderMainPlot()
{
    static float old_time_ammount = time_ammount;
    auto old_plot_style = ImPlot::GetStyle();
    ImPlot::GetStyle().MarkerSize = marker_size;
    ImPlot::GetStyle().DigitalBitHeight = DigitalBitHeight;

    auto plot_window_height = ImGui::GetCurrentWindow()->Size.y - (ImGui::GetStyle().ItemSpacing.y * 4.f);
    ImGui::BeginChild("Plot view", ImVec2(0.f, (plot_window_height * 0.65f) - (ImGui::GetStyle().ItemSpacing.y * 1.25f)), true);
    if (ImPlot::BeginPlot("##Digital", ImVec2(-1.f, -1.f), 0)) {
        ImPlot::SetupAxes(0, 0, 0, auto_size ? ImPlotAxisFlags_AutoFit : 0);
        if (!paused || old_time_ammount != time_ammount) {
            ImPlot::SetupAxisLimits(ImAxis_X1, time - time_ammount, time, ImGuiCond_Always);
            old_time_ammount = time_ammount;
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, time - time_ammount, time, ImGuiCond_Appearing);
        }

        for (auto& iter : inputs) {
            if (iter.data_buffer_analog.Data.size() > 0)
            {
                ImPlot::PushStyleColor(ImPlotCol_Line, iter.plot_color);
                ImPlot::PlotLine(iter.input_name.c_str(), &iter.data_buffer_analog.Data[0].x, &iter.data_buffer_analog.Data[0].y, iter.data_buffer_analog.Data.size(), iter.data_buffer_analog.Offset, 2 * sizeof(float));
                ImPlot::PopStyleColor();
                if (show_sampling && iter.data_buffer_sampling.Data.size() > 0) {
                    if (sample_show_type == 0) {
                        ImPlot::PlotScatter((iter.input_name + std::string(" sample points")).c_str(), &iter.data_buffer_sampling.Data[0].x, &iter.data_buffer_sampling.Data[0].y, iter.data_buffer_sampling.Data.size(), iter.data_buffer_sampling.Offset, 2 * sizeof(float));
                    }
                    else if (sample_show_type == 1)
                    {
                        ImPlot::PlotStems((iter.input_name + std::string(" sample points")).c_str(), &iter.data_buffer_sampling.Data[0].x, &iter.data_buffer_sampling.Data[0].y, iter.data_buffer_sampling.Data.size(), 0, iter.data_buffer_sampling.Offset, 2 * sizeof(float));
                    }
                }

                if (show_quant_data && iter.data_buffer_quantization.Data.size() > 0) {
                    if (quant_show_type == 0) {
                        ImPlot::PlotStairs((iter.input_name + std::string(" quantizied")).c_str(), &iter.data_buffer_quantization.Data[0].x, &iter.data_buffer_quantization.Data[0].y, iter.data_buffer_quantization.Data.size(), iter.data_buffer_quantization.Offset, 2 * sizeof(float));
                    }
                    else if (quant_show_type == 1) {
                        ImPlot::PlotLine((iter.input_name + std::string(" quantizied")).c_str(), &iter.data_buffer_quantization.Data[0].x, &iter.data_buffer_quantization.Data[0].y, iter.data_buffer_quantization.Data.size(), iter.data_buffer_quantization.Offset, 2 * sizeof(float));
                    }
                }

                if (show_digital_data && iter.data_buffer_quantization_index.Data.size() > 0 && iter.data_buffer_quantization.Data.size() > 0 && iter.data_buffer_quantization_binary.Data.size() > 0) {
                    //render text
                    auto last_index_value = ImVec2(0, 0);
                    if (iter.data_buffer_quantization_index.Offset == 0)
                        last_index_value = iter.data_buffer_quantization_index.Data.back();
                    else {
                        auto data_index = (iter.data_buffer_quantization_index.Offset - 1) % iter.data_buffer_quantization_index.MaxSize;
                        last_index_value = iter.data_buffer_quantization_index.Data[data_index];
                    }

                    auto last_value = ImVec2(0, 0);
                    if (iter.data_buffer_quantization.Offset == 0)
                        last_value = iter.data_buffer_quantization.Data.back();
                    else {
                        auto data_index = (iter.data_buffer_quantization.Offset - 1) % iter.data_buffer_quantization.MaxSize;
                        last_value = iter.data_buffer_quantization.Data[data_index];
                    }

                    if (show_digital_data_text) {
                        if (digital_data_type == 0)
                        {
                            ImPlot::Annotation(last_index_value.x, last_value.y, ImVec4(1.f, 1.f, 1.f, 0.4f), ImVec2(-5.f, -5.f), true, "%s", NumberToBitString(last_index_value.y).c_str());
                        }
                        else if (digital_data_type == 1) {
                            ImPlot::Annotation(last_index_value.x, last_value.y, ImVec4(1.f, 1.f, 1.f, 0.4f), ImVec2(-5.f, -5.f), true, "%s", NumberToHexString(last_index_value.y).c_str());
                        }
                        else if (digital_data_type == 2) {
                            ImPlot::Annotation(last_index_value.x, last_value.y, ImVec4(1.f, 1.f, 1.f, 0.4f), ImVec2(-5.f, -5.f), true, "%d", (int)last_index_value.y);
                        }
                    }

                    //render tooltip
                    auto size = iter.data_buffer_quantization_index.Offset == 0 ? (iter.data_buffer_quantization_index.Data.size()) : (iter.data_buffer_quantization_index.Offset % iter.data_buffer_quantization_index.MaxSize);
                    PlotDataTooltip(iter.input_name.c_str(), &iter.data_buffer_quantization_index.Data[0].x, &iter.data_buffer_quantization_index.Data[0].y, 0.25f, size, 2 * sizeof(float));

                    //render digital binary plot
                    ImPlot::PlotDigital((iter.input_name + std::string(" digital")).c_str(), &iter.data_buffer_quantization_binary.Data[0].x, &iter.data_buffer_quantization_binary.Data[0].y, iter.data_buffer_quantization_binary.Data.size(), iter.data_buffer_quantization_binary.Offset, 2 * sizeof(float), show_digital_data_text);
                }
            }
        }
        int draw_line_index = 0;
        if (show_quant_levels) {
            int number_of_positions = (int)pow(2, quant_bit_depth);
            float min_max_diff = (float)abs(max_quant_value - min_quant_value);
            if (number_of_positions != 0 && number_of_positions < 2000) {
                float qunat_step = min_max_diff / (float)number_of_positions;

                double base = max_quant_value > min_quant_value ? min_quant_value : max_quant_value;
                for (int i = 0; i < number_of_positions; i++) {
                    double pos = (float)(base + i * qunat_step);
                    ImPlot::DragLineY(draw_line_index++, &pos, ImVec4(1, 1, 1, 0.25f), 1, ImPlotDragToolFlags_NoInputs);
                }
            }
        }

        if (show_quant_limits) {
            ImPlot::DragLineY(draw_line_index++, &min_quant_value, ImVec4(1, 1, 1, 1), 1, ImPlotDragToolFlags_NoFit);
            ImPlot::TagY(min_quant_value, ImVec4(1, 1, 1, 1), "Min");

            ImPlot::DragLineY(draw_line_index++, &max_quant_value, ImVec4(1, 1, 1, 1), 1, ImPlotDragToolFlags_NoFit);
            ImPlot::TagY(max_quant_value, ImVec4(1, 1, 1, 1), "Max");
        }
        ImPlot::EndPlot();
    }

    ImPlot::GetStyle() = old_plot_style;
}

template <typename T>
int BinarySearch(const T* arr, int r, T x, int stride) {
    int best_index = 0;
    float best_diff = FLT_MAX;

    for (int i = 0; i < r; i++) {
        auto item = *(T*)((uintptr_t)arr + (uintptr_t)(stride * i));
        float diff = fabs(x - item);

        if (diff <= best_diff && x > item) {
            best_index = i;
            best_diff = diff;
        }
    }

    return best_index;
}

void PlotManager::PlotDataTooltip(const char* name, const float* xs, const float* value, float width_percent, int count, int stride) {

    // get ImGui window DrawList
    ImDrawList* draw_list = ImPlot::GetPlotDrawList();
    // calc real value width
    double half_width = width_percent / 2.f;

    // custom tool
    if (ImPlot::IsPlotHovered()) {
        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        float  tool_l = ImPlot::PlotToPixels(mouse.x - half_width, mouse.y).x;
        float  tool_r = ImPlot::PlotToPixels(mouse.x + half_width, mouse.y).x;
        float  tool_t = ImPlot::GetPlotPos().y;
        float  tool_b = tool_t + ImPlot::GetPlotSize().y;
        ImPlot::PushPlotClipRect();
        draw_list->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), IM_COL32(128, 128, 128, 64));
        ImPlot::PopPlotClipRect();
        // find mouse location index
        int idx = BinarySearch<float>(xs, count, (float)mouse.x, stride);
        // render tool tip (won't be affected by plot clip rect)
        if (idx != -1) {
            ImGui::BeginTooltip();

            if (digital_data_type == 0)
            {
                ImGui::Text("%s: %s", name, NumberToBitString(*(float*)((uintptr_t)value + (unsigned long)idx * (unsigned long)stride)).c_str());
            }
            else if (digital_data_type == 1) {
                ImGui::Text("%s: %s", name, NumberToHexString(*(float*)((uintptr_t)value + (unsigned long)idx * (unsigned long)stride)).c_str());
            }
            else if (digital_data_type == 2) {
                ImGui::Text("%s: %d", name, (int)*(float*)((uintptr_t)value + (unsigned long)idx * (unsigned long)stride));
            }
            ImGui::EndTooltip();
        }
    }
}

void PlotManager::RenderDigitalizationOtions()
{
    ImGui::NewLine();
    ImGui::Text("Sampling frequency:");
    ImGui::PushItemWidth(-1.f);
    ImGui::SliderFloat("##SamplingRate", &sampling_rate, 1.f, 20.f, "%.2f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Sampling frequency determins, how many times per give second we should take a sample");
    ImGui::PopItemWidth();
    ImGui::Checkbox("Show sampling", &show_sampling);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Render sample points in main plot");
    ImGui::Checkbox("Sync sampling time", &sync_sample_timing);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Sync sample timing delta for all inputs");
    ImGui::NewLine();
    ImGui::Checkbox("Add noise", &add_noise);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("You can add additional noise exclusivly just for the sampled signal");
    if (add_noise) {
        ImGui::Text("Noise multiplier:");
        ImGui::PushItemWidth(-1.f);
        ImGui::SliderFloat("##NoiseMultiplier", &noise_multiplier, 0.2f, 5.f, "%.2f");
        ImGui::PopItemWidth();
    }

    ImGui::NewLine();
    ImGui::Text("Quantization area:");
    ImGui::InputDouble("Min", &min_quant_value, 0.01, 0.1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This option sets the minimum limit of quantization process");
    ImGui::InputDouble("Max", &max_quant_value, 0.01, 0.1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This option sets the maximum limit of quantization process");
    ImGui::Checkbox("Show quantiz. limits", &show_quant_limits);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("You can change limits by dragging them on the main plot, or by changing thier decimal value above.");
    ImGui::Checkbox("Show quantiz. levels", &show_quant_levels);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Shows each quantization level on main plot");
    ImGui::Text("Quantization bit depth:");
    ImGui::PushItemWidth(-1.f);
    ImGui::SliderInt("##QuantizationBitDepth", &quant_bit_depth, 2, 10);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This changes the resolution of the quantization process");
    ImGui::Checkbox("Show quantizied data", &show_quant_data);
    ImGui::NewLine();
    ImGui::Checkbox("Show digital data", &show_digital_data);
    ImGui::Checkbox("Show digital anotation", &show_digital_data_text);
    ImGui::Text("Data type:");
    ImGui::Combo("##digital_data_type", &digital_data_type, "Binary\0Hexadeciaml\0Decimal\0\0");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This changes the output format of 'Show digital anotation' option");

    ImGui::PopItemWidth();
}

void PlotManager::RenderMainPlotSettings()
{
    ImGui::Columns(2);
    ImGui::Checkbox("Pause!", &paused);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This allows you to stop time, and move with X axes freely");
    ImGui::Text("Time scale:");
    ImGui::PushItemWidth(-1.f);
    ImGui::SliderFloat("##TimeScale", &time_scale, 0.1f, 2.f, "%.2f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Allows you to change time scale. Time = real_time * time_scale");
    ImGui::Text("Sample point size:");
    ImGui::SliderFloat("##Samplepointsize", &marker_size, 2.f, 5.f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::Checkbox("Auto size axes", &auto_size);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This option will resize Y axes acordingly to the input");
    ImGui::Text("Show x seconds:");
    ImGui::PushItemWidth(-1.f);
    ImGui::SliderFloat("##ShowXSeconds", &time_ammount, 0.01f, 20.f, "%.2f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("This changes how many seoncds are shown on X axes");
    ImGui::PopItemWidth();
    ImGui::Text("Digital plot height:");
    ImGui::PushItemWidth(-1.f);
    ImGui::SliderFloat("##digitalPlotHeight", &DigitalBitHeight, 5.f, 50.f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hight of invidual digital plot data");
    ImGui::PopItemWidth();
    ImGui::Text("Sample data render style:");
    ImGui::PushItemWidth(-1.f);
    ImGui::Combo("##SampleDataStyle", &sample_show_type, "Scatter\0Stem\0\0");
    ImGui::Text("Quantizied data render style:");
    ImGui::Combo("##quantiziedDataStyle", &quant_show_type, "Stairstep\0Line\0\0");
    ImGui::PopItemWidth();
    ImGui::Columns(1);
}

#ifdef __EMSCRIPTEN__
EM_JS(void, downloadStringAsFile, (const char* data, int datalen), {
    // Create a download link
    var link = document.createElement('a');
    link.download = 'data';
    link.href = 'data:text/csv;charset=utf-8,' + encodeURIComponent(UTF8ToString(data, datalen));

    // Click on the link to trigger the download
    link.click();
    });

static void downloadStringAsFile(std::string data, std::string filename) {
    downloadStringAsFile(data.c_str(), data.size());
}
#endif

void PlotManager::RenderTextOutput()
{
    ImGui::Text("Selected:");
    ImGui::SameLine();

    static int selected_format = 0;
    static std::string selected_string = "None";
    static int selected_index = 0;
    if (inputs.size() <= 0 && selected_index >= 0) {
        selected_index = -1;
        selected_string = "None";
    }

    if (selected_index == -1 && inputs.size() > 0) {
        selected_index = 0;
    }

    if (selected_index >= 0 && selected_index < inputs.size())
        selected_string = inputs[selected_index].input_name;

    if (ImGui::BeginCombo("##Selection", selected_string.c_str(), 0)) {
        for (int n = 0; n < inputs.size(); n++)
        {
            const bool is_selected = (selected_index == n);
            if (ImGui::Selectable(inputs[n].input_name.c_str(), is_selected))
                selected_index = n;

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);

    ImGui::BeginChild("##outputText",
        ImVec2(0, ImGui::CalcTextSize("TEST STRING").y +
            ImGui::GetStyle().WindowPadding.y * 2 +
            ImGui::GetStyle().ScrollbarSize),
        true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        if (selected_index < inputs.size()) {
            auto& in = inputs[selected_index];
            std::string out = "";

            int last_data_index = 0;
            int first_data_index = 0;
            if (in.data_buffer_quantization_index.Offset != 0) {
                last_data_index = (in.data_buffer_quantization_index.Offset - 1) % in.data_buffer_quantization_index.MaxSize;
                first_data_index = last_data_index - in.data_buffer_quantization_index.MaxSize;
                if (first_data_index < 0)
                {
                    first_data_index = in.data_buffer_quantization_index.MaxSize + first_data_index;
                }
            }

            for (auto i = 0; i < in.data_buffer_quantization_index.MaxSize; i++) {
                int correct_index = 0;
                if (in.data_buffer_quantization_index.Offset != 0) {
                    correct_index = first_data_index + i;
                    if (correct_index >= in.data_buffer_quantization_index.MaxSize)
                        correct_index -= in.data_buffer_quantization_index.MaxSize;
                }
                else {
                    correct_index = i;

                    //sanity check
                    if (i >= in.data_buffer_quantization_index.Data.size())
                        break;
                }


                auto& iter = in.data_buffer_quantization_index.Data[correct_index];
                if (selected_format == 2) {
                    out += " ";
                    out += NumberToBitString(iter.y);
                }
                else if (selected_format == 1) {
                    out += " ";
                    out += NumberToHexString(iter.y);
                }
                else if (selected_format == 0) {
                    out += " ";
                    char str[32];
                    sprintf(str, "%0.f", iter.y);

                    out += str;
                }
            }

            ImGui::TextUnformatted(out.c_str());
            ImGui::SameLine();
            ImGui::SetScrollHereX(1.0f);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    static bool use_dot = false;
    ImGui::Text("Export data format:");
    ImGui::Combo("##formatexport", &selected_format, "Decimal\0Hexadecimal\0binary\0\0");
    if (ImGui::Button("Export data")) {
#ifndef __EMSCRIPTEN__
        ImGuiFileDialog::Instance()->OpenModal("Save as##export_output_data", "Save file", ".csv", ".");
#else
        ExportDataToFile(selected_index, "data", use_dot, selected_format);
#endif
    }

    ImGui::SameLine();
    ImGui::Checkbox("Use dot as decimal place", &use_dot);

    // display
    if (ImGuiFileDialog::Instance()->Display("Save as##export_output_data", 32, ImVec2(400, 300)))
    {
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            //save data to the file
            ExportDataToFile(selected_index, filePathName, use_dot, selected_format);
        }

        // close
        ImGuiFileDialog::Instance()->Close();
    }
}

void PlotManager::LoadJsonData(nlohmann::json& data)
{
    inputs.clear();
    if (data.is_null()) {
        return;
    }

    // Loop over the json array of settings
    if (data.contains("signals")) {
        for (nlohmann::json json_setting : data["signals"])
        {
            Signal in;

            in.input_name = json_setting.value("input_name", in.input_name.c_str());
            in.plot_color.x = json_setting.value("plot_color_x", in.plot_color.x);
            in.plot_color.y = json_setting.value("plot_color_y", in.plot_color.y);
            in.plot_color.z = json_setting.value("plot_color_z", in.plot_color.z);
            in.plot_color.w = json_setting.value("plot_color_w", in.plot_color.w);
            in.type = (PlotManager::function_names)json_setting.value("type", (int)in.type);
            in.math_expr = json_setting.value("math_expr", in.math_expr.c_str());
            in.amplitude = json_setting.value("amplitude", in.amplitude);
            in.noise = json_setting.value("noise", in.noise);
            inputs.push_back(in);
        }
    }

    marker_size = data.value("marker_size", 3.2f);
    auto_size = data.value("auto_size", true);
    sampling_rate = data.value("sampling_rate", 1.5f);
    show_sampling = data.value("show_sampling", false);
    add_noise = data.value("add_noise", false);
    noise_multiplier = data.value("noise_multiplier", 1.f);
    sample_show_type = data.value("sample_show_type", 0);
    max_quant_value = data.value("max_quant_value", 1.f);
    min_quant_value = data.value("min_quant_value", -1.f);
    show_quant_limits = data.value("show_quant_limits", false);
    quant_bit_depth = data.value("quant_bit_depth", 4);
    show_quant_data = data.value("show_quant_data", false);
    show_quant_levels = data.value("show_quant_levels", false);
    quant_show_type = data.value("show_quant_levels", 0);
    show_digital_data = data.value("show_digital_data", false);
    digital_data_type = data.value("digital_data_type", 0);
    paused = data.value("paused", false);
    time_scale = data.value("time_scale", 1.f);
    DigitalBitHeight = data.value("DigitalBitHeight", 25.f);
    sync_sample_timing = data.value("sync_sample_timing", false);
    show_digital_data_text = data.value("show_digital_data_text", false);
    time_ammount = data.value("time_ammount", 10.f);
}

nlohmann::json PlotManager::GetJsonData()
{
    nlohmann::json data;
    for (const auto& iter : inputs) {
        nlohmann::json item;
        item["input_name"] = iter.input_name;
        item["plot_color_x"] = iter.plot_color.x;
        item["plot_color_y"] = iter.plot_color.y;
        item["plot_color_z"] = iter.plot_color.z;
        item["plot_color_w"] = iter.plot_color.w;
        item["type"] = (int)iter.type;
        item["math_expr"] = iter.math_expr;
        item["amplitude"] = iter.amplitude;
        item["noise"] = iter.noise;
        data["signals"].push_back(item);
    }

    data["marker_size"] = marker_size;
    data["auto_size"] = auto_size;
    data["sampling_rate"] = sampling_rate;
    data["show_sampling"] = show_sampling;
    data["add_noise"] = add_noise;
    data["noise_multiplier"] = noise_multiplier;
    data["sample_show_type"] = sample_show_type;
    data["max_quant_value"] = max_quant_value;
    data["min_quant_value"] = min_quant_value;
    data["show_quant_limits"] = show_quant_limits;
    data["quant_bit_depth"] = quant_bit_depth;
    data["show_quant_data"] = show_quant_data;
    data["show_quant_levels"] = show_quant_levels;
    data["quant_show_type"] = quant_show_type;
    data["show_digital_data"] = show_digital_data;
    data["digital_data_type"] = digital_data_type;
    data["paused"] = paused;
    data["time_scale"] = time_scale;
    data["DigitalBitHeight"] = DigitalBitHeight;
    data["sync_sample_timing"] = sync_sample_timing;
    data["show_digital_data_text"] = show_digital_data_text;
    data["time_ammount"] = time_ammount;

    return data;
}

void PlotManager::ExportDataToFile(int data_input_index, std::string filePathName, bool use_dot, int data_format) {
    if (data_input_index >= inputs.size()) {
        printf("Error while saving file: selected plot was invalid!");
        return;
    }
    std::string out = "";
    std::string separator = ";";
    char original_symbol = '.';
    char desired_symbol = ',';

    try {
#ifdef __EMSCRIPTEN__
        std::stringstream file;
#else
        // Open the file for writing
        std::ofstream file(filePathName);
#endif

        file << "x" << separator << " y" << "\n";
        auto& in = inputs[data_input_index];
        int last_data_index = 0;
        int first_data_index = 0;
        if (in.data_buffer_quantization_index.Offset != 0) {
            last_data_index = (in.data_buffer_quantization_index.Offset - 1) % in.data_buffer_quantization_index.MaxSize;
            first_data_index = last_data_index - in.data_buffer_quantization_index.MaxSize;
            if (first_data_index < 0)
            {
                first_data_index = in.data_buffer_quantization_index.MaxSize + first_data_index;
            }
        }

        for (auto i = 0; i < in.data_buffer_quantization_index.MaxSize; i++) {
            int correct_index = 0;
            if (in.data_buffer_quantization_index.Offset != 0) {
                correct_index = first_data_index + i;
                if (correct_index > in.data_buffer_quantization_index.MaxSize)
                    correct_index -= in.data_buffer_quantization_index.MaxSize;
            }
            else {
                correct_index = i;

                if (i >= in.data_buffer_quantization_index.Data.size())
                    break;
            }

            auto& iter = in.data_buffer_quantization_index.Data[correct_index];

            std::string x_str = std::to_string(iter.x);
            std::string y_str = "";

            if (data_format == 0) {
                y_str = std::to_string((int)iter.y);
            }
            else if (data_format == 1) {
                y_str = NumberToHexString(iter.y);
            }
            else if (data_format == 2) {
                y_str = NumberToBitString(iter.y);
            }

            if (!use_dot) {
                // Convert the decimal point symbols using the replace function
                std::replace(x_str.begin(), x_str.end(), original_symbol, desired_symbol);

                if (data_format == 0) {
                    std::replace(y_str.begin(), y_str.end(), original_symbol, desired_symbol);
                }
            }

            // Write the data to the file
            file << x_str << separator << y_str << "\n";
        }

#ifdef __EMSCRIPTEN__
        out = file.str();
        downloadStringAsFile(out, filePathName);//filePathName is used as name of the file
#else
        // Close the file
        file.close();
#endif
    }
    catch (const std::exception& ex) {
        printf("Error while saving file: %s", ex.what());
    }
}

void PlotManager::OpenEditInputDialog()
{
    ImGui::OpenPopup("Edit input");
}

void PlotManager::RenderEditInputDialog()
{
    static bool just_opne = true;
    if (ImGui::BeginPopupModal("Edit input", NULL))
    {
        if (edit_input == -1)
            ImGui::CloseCurrentPopup();

        if (edit_input >= 0 && edit_input < inputs.size()) {
            static char name[256] = "Analog_0\0";
            static char math_expression[500] = "\0";
            static int item = 0;
            static float color[4] = { 0.4f, 0.7f, 0.0f, 1.f };
            static float amplitude = 1.f;
            static float noise = 0.f;

            auto& temp = inputs[edit_input];
            if (just_opne) {
                strncpy(name, temp.input_name.c_str(), temp.input_name.size() > 256 ? 255 : temp.input_name.size() + 1);
                name[255] = '\0';

                strncpy(math_expression, temp.math_expr.c_str(), temp.math_expr.size() > 500 ? 499 : temp.math_expr.size() + 1);
                math_expression[499] = '\0';

                item = temp.type;

                color[0] = temp.plot_color.x;
                color[1] = temp.plot_color.y;
                color[2] = temp.plot_color.z;
                color[3] = temp.plot_color.w;

                amplitude = temp.amplitude;
                noise = temp.noise;
            }

            ImGui::Text("Select input type:");
            ImGui::ComboV("##input_function", &item, vec_function_names);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Select how should we generate the signal");
            ImGui::Text("Input name:");
            ImGui::InputText("##InputNameInput", name, 256);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Name of the input signal, you will be able to see this in legend of the plot");
            ImGui::Text("Plot color:");
            ImGui::ColorEdit4("##PlotColor", color);

            if (item == PlotManager::function_names::math_expression) {
                ImGui::Text("Math expression:");
                ImGui::InputText("##MathExpressionInput", math_expression, 500);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Type here an mathematic expression, you can use variable x to define time. For ex. 'sin(x)'");
            }
            else {
                ImGui::Text("Amplitude:");
                ImGui::SliderFloat("##AmplitudeSlider", &amplitude, 0.01f, 1.f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("This allows you to multiply the amplitude of given function");
            }

            ImGui::Text("Noise:");
            ImGui::SliderFloat("##noiseEditInput", &noise, 0.00f, 5.f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("You can add noise to the signal to make it more realistic");

            just_opne = false;
            ImGui::Dummy(ImVec2(300.f, 15.f));

            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
                just_opne = true;
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.95f, 0.54f, 0.46f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.54f, 0.46f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.54f, 0.46f, 0.7f));
            if (ImGui::Button("Delete")) {
                DeleteInput(edit_input);
                ImGui::CloseCurrentPopup();

                ImGui::PopStyleColor(3);
                ImGui::EndPopup();
                just_opne = true;
                return;
            }

            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.31f, 0.98f, 0.48f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.31f, 0.98f, 0.48f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.41f, 0.99f, 0.57f, 0.7f));
            if (ImGui::Button("Save"))
            {
                auto new_name = std::string(name);
                if (CheckIfInputNameExists(new_name, edit_input))
                    new_name += std::string("#") + std::to_string(edit_input);

                temp.input_name = new_name;
                temp.math_expr = std::string(math_expression);
                temp.type = (PlotManager::function_names)item;
                temp.plot_color = ImVec4(color[0], color[1], color[2], color[3]);
                temp.amplitude = amplitude;
                temp.noise = noise;
                ImGui::CloseCurrentPopup();
                just_opne = true;
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::EndPopup();
    }
    else {
        just_opne = true;
    }
}

int PlotManager::RenderAnalogInputCard(Signal& input)
{
    int ret = 0;
    constexpr float aspect_ratio = 9.f / 16.f;
    auto window_width = ImGui::GetCurrentWindow()->Size.x;
    auto card_height = window_width * aspect_ratio;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::BeginChild(std::string(input.input_name + "##CARD").c_str(), ImVec2(0, card_height), true);
    {
        auto cursor_pos = ImGui::GetCursorPos();

        if (input.data_buffer_analog.Data.size() > 0) {
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
            if (ImPlot::BeginPlot(std::string(input.input_name + "##CARDPLOT").c_str(), ImVec2(window_width, card_height), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild | ImPlotFlags_NoInputs)) {
                ImPlot::SetupAxisLimits(ImAxis_X1, time - 10.0, time, ImGuiCond_Always);
                ImPlot::SetupAxes(0, 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_AutoFit);
                ImPlot::PushStyleColor(ImPlotCol_Line, input.plot_color);
                ImPlot::PlotLine(std::string(input.input_name + "##CARDPLOTLINE").c_str(), &input.data_buffer_analog.Data[0].x, &input.data_buffer_analog.Data[0].y, input.data_buffer_analog.Data.size(), input.data_buffer_analog.Offset, 2 * sizeof(float));
                ImPlot::PopStyleColor();
                ImPlot::EndPlot();
            }
            ImPlot::PopStyleVar();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.6f));
            ImGui::SetCursorPos(cursor_pos);
            ImGui::Button("Edit", ImVec2(-1.f, card_height / 2.f));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                ret = 1;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
            ImGui::SetCursorPos(cursor_pos + ImVec2(0, card_height / 2.f));
            ImGui::Button("Remove", ImVec2(-1.f, card_height / 2.f));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                ret = 2;
            }
            ImGui::PopStyleColor(4);
        }

        ImGui::SetCursorPos(cursor_pos + ImVec2(5, 5));
        ImGui::Text(input.input_name.c_str());
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();

    return ret;
}

float PlotManager::FindClosestQuantValue(float value, float quant_step, int number_of_positions, float min, float max) {
    float closest_value = FLT_MIN;
    float closest_diff = FLT_MAX;

    float base = max > min ? min : max;

    for (int i = 0; i < number_of_positions; i++) {
        auto pos = base + quant_step * i;
        auto diff = abs(value - pos);
        if (diff < closest_diff) {
            closest_diff = diff;
            closest_value = pos;
        }
    }

    return closest_value;
}

int PlotManager::FindClosestQuantIndex(float value, float quant_step, int number_of_positions, float min, float max) {
    int closest_index = 0;
    float closest_diff = FLT_MAX;

    float base = max > min ? min : max;

    for (int i = 0; i < number_of_positions; i++) {
        auto pos = base + quant_step * i;
        auto diff = abs(value - pos);
        if (diff < closest_diff) {
            closest_diff = diff;
            closest_index = i;
        }
    }

    return closest_index;
}

void PlotManager::GenerateQuantiziedValue(Signal& output, float last_y_axis_value, float qunat_step, int number_of_positions) {
    float quantizied_value = FindClosestQuantValue(last_y_axis_value, qunat_step, number_of_positions, (float)min_quant_value, (float)max_quant_value);
    output.data_buffer_quantization.AddPoint(time, quantizied_value);
}

void PlotManager::GenerateQuantiziedIndex(Signal& output, float last_y_axis_value, float qunat_step, int number_of_positions) {
    int quant_index = FindClosestQuantIndex(last_y_axis_value, qunat_step, number_of_positions, (float)min_quant_value, (float)max_quant_value);
    output.data_buffer_quantization_index.AddPoint(time, (float)quant_index);
}

void PlotManager::GenerateQuantiziedBinary(Signal& output, float time_delta) {
    auto decimal_to_binary = [](int decimal_value, int num_bits) {
        std::vector<bool> binary(num_bits);

        for (int i = 0; i < num_bits; i++) {
            binary[num_bits - 1 - i] = (decimal_value >> i) & 1;
        }

        return binary;
    };
    //we are generating value for previous tick only, so if there is not any, we can't do this!
    if (output.data_buffer_quantization_index.Data.size() <= 0)
        return;

    //get last index value
    auto last_index_value = ImVec2(0, 0);
    if (output.data_buffer_quantization_index.Offset == 0)
        last_index_value = output.data_buffer_quantization_index.Data.back();
    else {
        auto data_index = (output.data_buffer_quantization_index.Offset - 1) % output.data_buffer_quantization_index.MaxSize;
        last_index_value = output.data_buffer_quantization_index.Data[data_index];
    }

    //convert index value to array of 1 and 0
    auto binary = decimal_to_binary((int)last_index_value.y, quant_bit_depth);

    //generate x ticks for each bit depth of the quantizier
    auto bit_tick_size_ms = time_delta / quant_bit_depth;
    for (int o = 0; o < quant_bit_depth && o < binary.size(); o++)
    {
        //get correct time of tick
        auto binary_data_time = time - time_delta + (bit_tick_size_ms * o);

        int value = (int)binary[o];

        //this will mark for us starting position of each invidual binuar number
        if (o == 0) {   
            if (value == 1)
                value = 2;
            else if (value == 0)
                value = -1;
        }

        output.data_buffer_quantization_binary.AddPoint(binary_data_time, (float)value);
    }
}

void PlotManager::TickOutputData(Signal& output)
{
    auto last_sampletime = output.last_sample_time;
    if (sync_sample_timing)
        last_sampletime = last_sampled_time_global;

    auto sampling_rate_in_ms = 1.f / (float)sampling_rate;
    if (last_sampletime + sampling_rate_in_ms < time) {
        auto time_delta = time - output.last_sample_time;
        //sampling
        output.last_sample_time = time;
        //get last valid analog value
        float last_y_axis_value = 0.f;
        if (output.data_buffer_analog.Offset == 0)
            last_y_axis_value = output.data_buffer_analog.Data.back().y;
        else {
            auto data_index = (output.data_buffer_analog.Offset - 1) % output.data_buffer_analog.MaxSize;
            last_y_axis_value = output.data_buffer_analog.Data[data_index].y;
        }

        //add guassian noise
        if (add_noise)
        {
            bool add_or_decreese = rand() % 2;
            if (add_or_decreese)
                last_y_axis_value += NoiseGenerator::GenerateGussianNoise() * noise_multiplier;
            else
                last_y_axis_value -= NoiseGenerator::GenerateGussianNoise() * noise_multiplier;
        }

        //sampled analog data
        output.data_buffer_sampling.AddPoint(time, last_y_axis_value);

        //quantization
        int number_of_positions = (int)pow(2, quant_bit_depth);
        float min_max_diff = (float)abs(max_quant_value - min_quant_value);
        if (number_of_positions != 0) {
            float qunat_step = min_max_diff / (float)number_of_positions;

            GenerateQuantiziedValue(output, last_y_axis_value, qunat_step, number_of_positions);

            //generate binary data for previous tick first!!
            GenerateQuantiziedBinary(output, time_delta);

            GenerateQuantiziedIndex(output, last_y_axis_value, qunat_step, number_of_positions);
        };
    }
}

void bin(unsigned n, std::string& str)
{
    if (n > 1)
        bin(n >> 1, str);

    if (n & 1)
        str += '1';
    else
        str += '0';
}

std::string PlotManager::NumberToBitString(float number)
{
    int int_num = (int)number;
    unsigned int un_int = abs(int_num);
    std::string out = "";
    bin(un_int, out);
    if (out.length() < quant_bit_depth)
    {
        std::string prefix = "";
        auto diff = quant_bit_depth - out.length();
        for (int i = 0; i < diff; i++) {
            prefix += "0";
        }
        out = prefix + out;
    }
    return out;
}

std::string PlotManager::NumberToHexString(float number)
{
    int int_num = (int)number;
    unsigned int un_int = abs(int_num);
    std::stringstream stream;
    stream << std::hex << un_int;
    std::string result = stream.str();

    //to upper
    for (auto& iter : result) {
        iter = toupper(iter);
    }

    result = "0x" + result;

    return result;
}

bool PlotManager::CheckIfInputNameExists(std::string name, int skip)
{
    bool ret = false;
    for (size_t i = 0; i < inputs.size(); i++) {
        if (skip == (int)i)
            continue;

        auto& iter = inputs[i];

        if (iter.input_name == name) {
            ret = true;
            break;
        }
    }
    return ret;
}

float PlotManager::ProcessMathExpression(Signal& input) {
    float out = 0.f;
    static parser_t parser;

    symbol_table_t symbol_table;
    symbol_table.add_variable("x", time);
    symbol_table.add_constants();

    expression_t expression;
    expression.register_symbol_table(symbol_table);

    if (parser.compile(input.math_expr, expression))
    {
        float result = expression.value();
        return result;
    }
    else {
        printf("Error in expression\n.");
    }
    return 0.f;
}

void PlotManager::TickInputData(Signal& input)
{
    float output = 0;
    switch (input.type) {
    case PlotManager::function_names::sin:
        output = sinf(time);
        break;
    case PlotManager::function_names::cos:
        output = cosf(time);
        break;
    case PlotManager::function_names::saw:
        output = atanf(tanf(time));
        break;
    case PlotManager::function_names::sqare:
        output = sinf(time) > 0.f ? 1.f : -1.f;
        break;
    case PlotManager::function_names::random:
        input.math_expr = "sin(x) + sin(2x) + sin(0.5*x) * cos(x)";
        output = ProcessMathExpression(input);
        break;
    case PlotManager::function_names::math_expression:
        output = ProcessMathExpression(input);
        break;
    }

    //change amplitude
    if (input.type != PlotManager::function_names::math_expression)
        output *= input.amplitude;

    //add noise
    if (input.noise != 0.f)
    {
        output += input.noise_gen.GetNoiseData() * input.noise;
    }

    input.data_buffer_analog.AddPoint(time, output);
}

void PlotManager::DeleteInput(size_t index)
{
    inputs.erase(inputs.begin() + index);
}

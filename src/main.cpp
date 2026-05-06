#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

static constexpr int APP_W = 820;
static constexpr int APP_H = 740;
static constexpr int STAGE_COUNT = 4;

using CheckGroup = std::array<bool, STAGE_COUNT>;

struct StepRow
{
    int number = 0;
    std::vector<CheckGroup> groups;
};

static std::vector<StepRow> rows;
static int startStep = 50;
static int endStep = 100;
static int fixCount = 0;

static const char *saveFile = "stepchecklist.txt";

static CheckGroup EmptyGroup()
{
    return CheckGroup{false, false, false, false};
}

static int TotalGroupCount()
{
    return 1 + fixCount;
}

static std::string Pad3(int value)
{
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << value;
    return ss.str();
}

static std::string GroupName(int group)
{
    if (group == 0)
    {
        return "Main";
    }

    return "Fix " + std::to_string(group);
}

static void EnsureGroupCount()
{
    int target = TotalGroupCount();

    for (auto &row : rows)
    {
        while ((int)row.groups.size() < target)
        {
            row.groups.push_back(EmptyGroup());
        }

        if ((int)row.groups.size() > target)
        {
            row.groups.resize(target);
        }
    }
}

static bool IsMainDone(const StepRow &row)
{
    if (row.groups.empty())
    {
        return false;
    }

    return row.groups[0][0] &&
           row.groups[0][1] &&
           row.groups[0][2] &&
           row.groups[0][3];
}

static bool GroupHasAnyChecked(const StepRow &row, int group)
{
    if (group < 0 || group >= (int)row.groups.size())
    {
        return false;
    }

    for (int s = 0; s < STAGE_COUNT; s++)
    {
        if (row.groups[group][s])
        {
            return true;
        }
    }

    return false;
}

static int CountChecked()
{
    int total = 0;

    for (const auto &row : rows)
    {
        for (const auto &group : row.groups)
        {
            for (int s = 0; s < STAGE_COUNT; s++)
            {
                if (group[s])
                {
                    total++;
                }
            }
        }
    }

    return total;
}

static int CountTotalBoxes()
{
    int total = 0;

    for (const auto &row : rows)
    {
        total += (int)row.groups.size() * STAGE_COUNT;
    }

    return total;
}

static int CountMainDone()
{
    int total = 0;

    for (const auto &row : rows)
    {
        if (IsMainDone(row))
        {
            total++;
        }
    }

    return total;
}

static bool SaveRowsSilent()
{
    std::ofstream file(saveFile, std::ios::trunc);

    if (!file)
    {
        return false;
    }

    file << "version|5\n";
    file << "fixCount|" << fixCount << "\n";

    for (const auto &row : rows)
    {
        file << row.number;

        for (const auto &group : row.groups)
        {
            file << "|";

            for (int s = 0; s < STAGE_COUNT; s++)
            {
                file << (group[s] ? '1' : '0');
            }
        }

        file << "\n";
    }

    return true;
}

static void GenerateRows()
{
    if (startStep > endStep)
    {
        std::swap(startStep, endStep);
    }

    fixCount = 0;
    rows.clear();

    for (int i = startStep; i <= endStep; i++)
    {
        StepRow row;
        row.number = i;
        row.groups.push_back(EmptyGroup());
        rows.push_back(row);
    }

    SaveRowsSilent();
}

static void AddFix()
{
    if (rows.empty())
    {
        GenerateRows();
    }

    fixCount++;

    for (auto &row : rows)
    {
        row.groups.push_back(EmptyGroup());
    }

    SaveRowsSilent();
}

static void DeleteFix(int fixNumber)
{
    if (fixCount <= 0)
    {
        return;
    }

    if (fixNumber < 1)
    {
        fixNumber = 1;
    }

    if (fixNumber > fixCount)
    {
        fixNumber = fixCount;
    }

    int groupIndex = fixNumber;

    for (auto &row : rows)
    {
        if (groupIndex >= 1 && groupIndex < (int)row.groups.size())
        {
            row.groups.erase(row.groups.begin() + groupIndex);
        }
    }

    fixCount--;

    EnsureGroupCount();
    SaveRowsSilent();
}

static void DeleteLastFix()
{
    DeleteFix(fixCount);
}

static void SaveRows()
{
    SaveRowsSilent();
}

static bool ParseStepLine(const std::string &line)
{
    std::stringstream ss(line);
    std::string part;

    if (!std::getline(ss, part, '|'))
    {
        return false;
    }

    StepRow row;

    try
    {
        row.number = std::stoi(part);
    }
    catch (...)
    {
        return false;
    }

    while (std::getline(ss, part, '|'))
    {
        CheckGroup group = EmptyGroup();

        for (int s = 0; s < STAGE_COUNT && s < (int)part.size(); s++)
        {
            group[s] = part[s] == '1';
        }

        row.groups.push_back(group);
    }

    if (row.groups.empty())
    {
        row.groups.push_back(EmptyGroup());
    }

    rows.push_back(row);
    return true;
}

static bool LoadRows()
{
    std::ifstream file(saveFile);

    if (!file)
    {
        return false;
    }

    rows.clear();
    fixCount = 0;

    std::string line;
    bool hasVersionHeader = false;
    bool hasFixCountHeader = false;

    if (std::getline(file, line))
    {
        if (line.rfind("version|", 0) == 0)
        {
            hasVersionHeader = true;
        }
        else
        {
            ParseStepLine(line);
        }
    }

    if (hasVersionHeader && std::getline(file, line))
    {
        if (line.rfind("fixCount|", 0) == 0)
        {
            try
            {
                fixCount = std::max(0, std::stoi(line.substr(9)));
                hasFixCountHeader = true;
            }
            catch (...)
            {
                fixCount = 0;
            }
        }
        else
        {
            ParseStepLine(line);
        }
    }

    while (std::getline(file, line))
    {
        ParseStepLine(line);
    }

    if (!hasFixCountHeader)
    {
        int maxGroups = 1;

        for (const auto &row : rows)
        {
            maxGroups = std::max(maxGroups, (int)row.groups.size());
        }

        fixCount = std::max(0, maxGroups - 1);
    }

    EnsureGroupCount();

    if (!rows.empty())
    {
        startStep = rows.front().number;
        endStep = rows.back().number;
    }

    return true;
}

static void ResetRows()
{
    rows.clear();
    fixCount = 0;
    SaveRowsSilent();
}

static void ApplyModernDarkStyle()
{
    ImGuiStyle &style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(18, 18);
    style.FramePadding = ImVec2(12, 8);
    style.CellPadding = ImVec2(8, 8);
    style.ItemSpacing = ImVec2(10, 9);
    style.ItemInnerSpacing = ImVec2(8, 7);
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 18.0f;
    style.FrameRounding = 12.0f;
    style.PopupRounding = 14.0f;
    style.ScrollbarRounding = 999.0f;
    style.GrabRounding = 999.0f;
    style.TabRounding = 12.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4 *c = style.Colors;

    c[ImGuiCol_Text] = ImVec4(0.97f, 0.98f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.66f, 0.78f, 1.00f);

    c[ImGuiCol_WindowBg] = ImVec4(0.01f, 0.03f, 0.07f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.035f, 0.065f, 0.13f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.07f, 0.14f, 1.00f);

    c[ImGuiCol_Border] = ImVec4(0.19f, 0.29f, 0.48f, 1.00f);
    c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.11f, 0.22f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.22f, 0.42f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.13f, 0.29f, 0.55f, 1.00f);

    c[ImGuiCol_Button] = ImVec4(0.12f, 0.20f, 0.38f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.34f, 0.64f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.23f, 0.42f, 0.80f, 1.00f);

    c[ImGuiCol_Header] = ImVec4(0.12f, 0.20f, 0.38f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.34f, 0.64f, 1.00f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.42f, 0.80f, 1.00f);

    c[ImGuiCol_CheckMark] = ImVec4(0.20f, 1.00f, 0.58f, 1.00f);

    c[ImGuiCol_ScrollbarBg] = ImVec4(0.025f, 0.045f, 0.09f, 1.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.27f, 0.38f, 0.62f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.55f, 0.88f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.66f, 1.00f, 1.00f);

    c[ImGuiCol_TableHeaderBg] = ImVec4(0.06f, 0.10f, 0.20f, 1.00f);
    c[ImGuiCol_TableBorderStrong] = ImVec4(0.24f, 0.35f, 0.58f, 1.00f);
    c[ImGuiCol_TableBorderLight] = ImVec4(0.16f, 0.25f, 0.42f, 1.00f);
    c[ImGuiCol_TableRowBg] = ImVec4(0.035f, 0.065f, 0.13f, 1.00f);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(0.045f, 0.085f, 0.17f, 1.00f);
}

static void PushPrimaryButtonStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.22f, 0.96f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.62f, 0.38f, 1.00f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.16f, 0.82f, 1.00f));
}

static void PopPrimaryButtonStyle()
{
    ImGui::PopStyleColor(3);
}

static void PushSuccessButtonStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.48f, 0.28f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.04f, 0.65f, 0.38f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.02f, 0.40f, 0.24f, 1.00f));
}

static void PopSuccessButtonStyle()
{
    ImGui::PopStyleColor(3);
}

static void PushDangerButtonStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.56f, 0.12f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.76f, 0.18f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.62f, 0.12f, 0.20f, 1.00f));
}

static void PopDangerButtonStyle()
{
    ImGui::PopStyleColor(3);
}

static void CenterTextInCell(const char *text, ImVec4 color)
{
    float avail = ImGui::GetContentRegionAvail().x;
    float textW = ImGui::CalcTextSize(text).x;

    if (avail > textW)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - textW) * 0.5f);
    }

    ImGui::TextColored(color, "%s", text);
}

static void RenderTopPanel()
{
    ImGui::BeginChild("TopPanel", ImVec2(0, 112), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(9, 8));

    ImGui::TextColored(ImVec4(0.56f, 0.84f, 1.00f, 1.00f), "StepChecklist");
    ImGui::SameLine();
    ImGui::TextDisabled("Workflow tracker made by mrkwxopya");

    int checked = CountChecked();
    int total = CountTotalBoxes();

    ImGui::SameLine();
    ImGui::TextDisabled("Steps:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.56f, 0.84f, 1.00f, 1.00f), "%d", (int)rows.size());

    ImGui::SameLine();
    ImGui::TextDisabled("Fixes:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.88f, 0.76f, 1.00f, 1.00f), "%d", fixCount);

    ImGui::SameLine();
    ImGui::TextDisabled("Main:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.42f, 1.00f, 0.66f, 1.00f), "%d", CountMainDone());

    ImGui::SameLine();
    ImGui::TextDisabled("Checked:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.88f, 0.76f, 1.00f, 1.00f), "%d/%d", checked, total);

    ImGui::Spacing();

    ImGui::TextDisabled("Start");
    ImGui::SameLine();
    ImGui::PushItemWidth(76);
    ImGui::InputInt("##StartStep", &startStep, 0, 0);
    ImGui::PopItemWidth();

    ImGui::SameLine();

    ImGui::TextDisabled("End");
    ImGui::SameLine();
    ImGui::PushItemWidth(76);
    ImGui::InputInt("##EndStep", &endStep, 0, 0);
    ImGui::PopItemWidth();

    ImGui::SameLine();

    PushPrimaryButtonStyle();
    if (ImGui::Button("Generate", ImVec2(96, 36)))
    {
        GenerateRows();
    }
    PopPrimaryButtonStyle();

    ImGui::SameLine();
    if (ImGui::Button("Save", ImVec2(66, 36)))
    {
        SaveRows();
    }

    ImGui::SameLine();
    if (ImGui::Button("Load", ImVec2(66, 36)))
    {
        LoadRows();
    }

    ImGui::SameLine();

    PushDangerButtonStyle();
    if (ImGui::Button("Reset", ImVec2(66, 36)))
    {
        ResetRows();
    }
    PopDangerButtonStyle();

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

static void RenderStageChecks(StepRow &row, int rowIndex, int group)
{
    float boxW = 24.0f;
    float gap = 10.0f;
    float totalW = (boxW * STAGE_COUNT) + (gap * (STAGE_COUNT - 1));
    float avail = ImGui::GetContentRegionAvail().x;

    if (avail > totalW)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - totalW) * 0.5f);
    }

    for (int s = 0; s < STAGE_COUNT; s++)
    {
        ImGui::PushID(rowIndex * 1000 + group * 10 + s);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.035f, 0.07f, 0.14f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.25f, 0.42f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.16f, 0.32f, 0.52f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.20f, 1.00f, 0.58f, 1.00f));

        bool changed = ImGui::Checkbox("##check", &row.groups[group][s]);

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();

        if (changed)
        {
            SaveRowsSilent();
        }

        ImGui::PopID();

        if (s < STAGE_COUNT - 1)
        {
            ImGui::SameLine(0.0f, gap);
        }
    }
}

static void RenderStepHeaderCell(bool *pendingAddFix)
{
    ImGui::TableSetBgColor(
        ImGuiTableBgTarget_CellBg,
        ImGui::GetColorU32(ImVec4(0.07f, 0.12f, 0.23f, 1.00f)));

    float avail = ImGui::GetContentRegionAvail().x;
    float buttonW = 82.0f;

    if (avail > buttonW)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - buttonW) * 0.5f);
    }

    PushSuccessButtonStyle();
    if (ImGui::Button("Add Fix", ImVec2(buttonW, 28)))
    {
        *pendingAddFix = true;
    }
    PopSuccessButtonStyle();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    CenterTextInCell("Step", ImVec4(0.92f, 0.96f, 1.00f, 1.00f));
}

static void RenderGroupHeaderCell(int group, int *pendingDeleteFix)
{
    ImVec4 bg = group == 0
                    ? ImVec4(0.00f, 0.38f, 0.56f, 1.00f)
                    : ImVec4(0.38f, 0.16f, 0.86f, 1.00f);

    ImVec4 label = group == 0
                       ? ImVec4(0.55f, 0.95f, 1.00f, 1.00f)
                       : ImVec4(0.92f, 0.84f, 1.00f, 1.00f);

    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(bg));

    if (group > 0)
    {
        float avail = ImGui::GetContentRegionAvail().x;
        float buttonW = 82.0f;

        if (avail > buttonW)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - buttonW) * 0.5f);
        }

        ImGui::PushID(group);
        PushDangerButtonStyle();
        if (ImGui::Button("Delete", ImVec2(buttonW, 28)))
        {
            *pendingDeleteFix = group;
        }
        PopDangerButtonStyle();
        ImGui::PopID();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    }
    else
    {
        ImGui::Dummy(ImVec2(1.0f, 30.0f));
    }

    CenterTextInCell(GroupName(group).c_str(), label);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
    CenterTextInCell("P    C    B    S", ImVec4(0.90f, 0.94f, 1.00f, 0.95f));
}

static void RenderTable()
{
    ImGui::BeginChild("TablePanel", ImVec2(0, 0), true);

    if (rows.empty())
    {
        ImGui::TextDisabled("No steps generated. Enter Start / End and press Generate.");
        ImGui::EndChild();
        return;
    }

    EnsureGroupCount();

    bool pendingAddFix = false;
    int pendingDeleteFix = 0;

    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoSavedSettings;

    const int columnCount = 1 + TotalGroupCount();

    if (ImGui::BeginTable("StepChecklistTable", columnCount, flags, ImVec2(-1, -1)))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_WidthFixed, 112.0f);

        for (int g = 0; g < TotalGroupCount(); g++)
        {
            ImGui::TableSetupColumn(GroupName(g).c_str(), ImGuiTableColumnFlags_WidthFixed, 178.0f);
        }

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers, 90.0f);

        ImGui::TableSetColumnIndex(0);
        RenderStepHeaderCell(&pendingAddFix);

        for (int g = 0; g < TotalGroupCount(); g++)
        {
            ImGui::TableSetColumnIndex(1 + g);
            RenderGroupHeaderCell(g, &pendingDeleteFix);
        }

        for (int r = 0; r < (int)rows.size(); r++)
        {
            StepRow &row = rows[r];

            ImGui::TableNextRow(0, 42.0f);

            ImGui::TableSetColumnIndex(0);

            if (IsMainDone(row))
            {
                ImGui::TableSetBgColor(
                    ImGuiTableBgTarget_CellBg,
                    ImGui::GetColorU32(ImVec4(0.03f, 0.28f, 0.14f, 1.00f)));
                ImGui::TextColored(ImVec4(0.70f, 1.00f, 0.80f, 1.00f), "STEP %s", Pad3(row.number).c_str());
            }
            else
            {
                ImGui::TableSetBgColor(
                    ImGuiTableBgTarget_CellBg,
                    ImGui::GetColorU32(ImVec4(0.06f, 0.10f, 0.20f, 1.00f)));
                ImGui::TextColored(ImVec4(0.88f, 0.94f, 1.00f, 1.00f), "STEP %s", Pad3(row.number).c_str());
            }

            for (int g = 0; g < TotalGroupCount(); g++)
            {
                ImGui::TableSetColumnIndex(1 + g);

                if (GroupHasAnyChecked(row, g))
                {
                    ImGui::TableSetBgColor(
                        ImGuiTableBgTarget_CellBg,
                        ImGui::GetColorU32(g == 0
                                               ? ImVec4(0.02f, 0.20f, 0.26f, 0.95f)
                                               : ImVec4(0.13f, 0.08f, 0.28f, 0.95f)));
                }

                RenderStageChecks(row, r, g);
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    if (pendingAddFix)
    {
        AddFix();
    }

    if (pendingDeleteFix > 0)
    {
        DeleteFix(pendingDeleteFix);
    }
}

static void RenderApp()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("StepChecklistRoot", nullptr, flags);

    RenderTopPanel();

    ImGui::Spacing();

    RenderTable();

    ImGui::End();
}

int main()
{
    if (!glfwInit())
    {
        return 1;
    }

    const char *glslVersion = "#version 130";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(APP_W, APP_H, "StepChecklist", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f);

    ApplyModernDarkStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    if (!LoadRows())
    {
        GenerateRows();
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderApp();

        if (
            ImGui::IsKeyPressed(ImGuiKey_Delete) &&
            !ImGui::IsAnyItemActive() &&
            fixCount > 0)
        {
            DeleteLastFix();
        }

        ImGui::Render();

        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);

        glViewport(0, 0, displayW, displayH);
        glClearColor(0.01f, 0.03f, 0.07f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    SaveRowsSilent();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
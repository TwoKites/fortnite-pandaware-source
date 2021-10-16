#include "UnrealStructs.h"
#include "Canvas.h"

#define PI (3.141592653589793f)

#include "detours.h"
#include "UnrealStructs.h"
#include "ImGui/stb_truetype.h"
#include "UnrealStructs.h"

#pragma comment(lib, "detours.lib")

//#include "SpeedHack.h"
// TODO: put in another file, and rename to something better
template<class DataType>
class SpeedHack {
	DataType time_offset;
	DataType time_last_update;

	double speed_;

public:
	SpeedHack(DataType currentRealTime, double initialSpeed) {
		time_offset = currentRealTime;
		time_last_update = currentRealTime;

		speed_ = initialSpeed;
	}

	// TODO: put lock around for thread safety
	void setSpeed(DataType currentRealTime, double speed) {
		time_offset = getCurrentTime(currentRealTime);
		time_last_update = currentRealTime;

		speed_ = speed;
	}

	// TODO: put lock around for thread safety
	DataType getCurrentTime(DataType currentRealTime) {
		DataType difference = currentRealTime - time_last_update;

		return (DataType)(speed_ * difference) + time_offset;
	}
};


// function signature typedefs
typedef DWORD(WINAPI* GetTickCountType)(void);
typedef ULONGLONG(WINAPI* GetTickCount64Type)(void);

typedef BOOL(WINAPI* QueryPerformanceCounterType)(LARGE_INTEGER* lpPerformanceCount);

// globals
GetTickCountType   g_GetTickCountOriginal;
GetTickCount64Type g_GetTickCount64Original;
GetTickCountType   g_TimeGetTimeOriginal;    // Same function signature as GetTickCount

QueryPerformanceCounterType g_QueryPerformanceCounterOriginal;


const double kInitialSpeed = 1.0; // initial speed hack speed

//                                  (initialTime,      initialSpeed)
SpeedHack<DWORD>     g_speedHack(GetTickCount(), kInitialSpeed);
SpeedHack<ULONGLONG> g_speedHackULL(GetTickCount64(), kInitialSpeed);
SpeedHack<LONGLONG>  g_speedHackLL(0, kInitialSpeed); // Gets set properly in DllMain

// function prototypes

DWORD     WINAPI GetTickCountHacked(void);
ULONGLONG WINAPI GetTickCount64Hacked(void);

BOOL      WINAPI QueryPerformanceCounterHacked(LARGE_INTEGER* lpPerformanceCount);

DWORD     WINAPI KeysThread(LPVOID lpThreadParameter);

// functions

void MainGay()
{
	// TODO: split up this function for readability.

	HMODULE kernel32 = GetModuleHandleA(xorthis("Kernel32.dll"));
	HMODULE winmm = GetModuleHandleA(xorthis("Winmm.dll"));

	// TODO: check if the modules are even loaded.

	// Get all the original addresses of target functions
	g_GetTickCountOriginal = (GetTickCountType)GetProcAddress(kernel32, xorthis("GetTickCount"));
	g_GetTickCount64Original = (GetTickCount64Type)GetProcAddress(kernel32, xorthis("GetTickCount64"));

	g_TimeGetTimeOriginal = (GetTickCountType)GetProcAddress(winmm, xorthis("timeGetTime"));

	g_QueryPerformanceCounterOriginal = (QueryPerformanceCounterType)GetProcAddress(kernel32, xorthis("QueryPerformanceCounter"));

	// Setup the speed hack object for the Performance Counter
	LARGE_INTEGER performanceCounter;
	g_QueryPerformanceCounterOriginal(&performanceCounter);

	g_speedHackLL = SpeedHack<LONGLONG>(performanceCounter.QuadPart, kInitialSpeed);

	// Detour functions
	DetourTransactionBegin();

	DetourAttach((PVOID*)&g_GetTickCountOriginal, (PVOID)GetTickCountHacked);
	DetourAttach((PVOID*)&g_GetTickCount64Original, (PVOID)GetTickCount64Hacked);

	// Detour timeGetTime to the hacked GetTickCount (same signature)
	DetourAttach((PVOID*)&g_TimeGetTimeOriginal, (PVOID)GetTickCountHacked);

	DetourAttach((PVOID*)&g_QueryPerformanceCounterOriginal, (PVOID)QueryPerformanceCounterHacked);

	DetourTransactionCommit();
}

void setAllToSpeed(double speed) {
	g_speedHack.setSpeed(g_GetTickCountOriginal(), speed);

	g_speedHackULL.setSpeed(g_GetTickCount64Original(), speed);

	LARGE_INTEGER performanceCounter;
	g_QueryPerformanceCounterOriginal(&performanceCounter);

	g_speedHackLL.setSpeed(performanceCounter.QuadPart, speed);
}

PVOID TargetPawn = nullptr;

namespace HookFunctions {
	bool Init(bool NoSpread, bool CalcShot);
	bool NoSpreadInitialized = false;
	bool CalcShotInitialized = false;
}

DWORD WINAPI GetTickCountHacked(void) {
	return g_speedHack.getCurrentTime(g_GetTickCountOriginal());
}

ULONGLONG WINAPI GetTickCount64Hacked(void) {
	return g_speedHackULL.getCurrentTime(g_GetTickCount64Original());
}

BOOL WINAPI QueryPerformanceCounterHacked(LARGE_INTEGER* lpPerformanceCount) {
	LARGE_INTEGER performanceCounter;

	BOOL result = g_QueryPerformanceCounterOriginal(&performanceCounter);

	lpPerformanceCount->QuadPart = g_speedHackLL.getCurrentTime(performanceCounter.QuadPart);

	return result;
}

int x = 30;
int y = 20;


void MenuKey()
{
	if (GetAsyncKeyState(VK_INSERT)) {
		Settings.ShowMenu = !Settings.ShowMenu;
	}
}

bool firstS = false;

BOOL valid_pointer(DWORD64 address)
{
	if (!IsBadWritePtr((LPVOID)address, (UINT_PTR)8)) return TRUE;
	else return FALSE;
}

void RadarRange(float* x, float* y, float range)
{
	if (fabs((*x)) > range || fabs((*y)) > range)
	{
		if ((*y) > (*x))
		{
			if ((*y) > -(*x))
			{
				(*x) = range * (*x) / (*y);
				(*y) = range;
			}
			else
			{
				(*y) = -range * (*y) / (*x);
				(*x) = -range;
			}
		}
		else
		{
			if ((*y) > -(*x))
			{
				(*y) = range * (*y) / (*x);
				(*x) = range;
			}
			else
			{
				(*x) = -range * (*x) / (*y);
				(*y) = -range;
			}
		}
	}
}

void CalcRadarPoint(SDK::Structs::FVector vOrigin, int& screenx, int& screeny)
{
	SDK::Structs::FRotator vAngle = SDK::Structs::FRotator{ SDK::Utilities::CamRot.x, SDK::Utilities::CamRot.y, SDK::Utilities::CamRot.z };
	auto fYaw = vAngle.Yaw * PI / 180.0f;
	float dx = vOrigin.X - SDK::Utilities::CamLoc.x;
	float dy = vOrigin.Y - SDK::Utilities::CamLoc.y;

	float fsin_yaw = sinf(fYaw);
	float fminus_cos_yaw = -cosf(fYaw);

	float x = dy * fminus_cos_yaw + dx * fsin_yaw;
	x = -x;
	float y = dx * fminus_cos_yaw - dy * fsin_yaw;

	float range = (float)15.f;

	RadarRange(&x, &y, range);

	ImVec2 DrawPos = ImGui::GetCursorScreenPos();
	ImVec2 DrawSize = ImGui::GetContentRegionAvail();

	int rad_x = (int)DrawPos.x;
	int rad_y = (int)DrawPos.y;

	float r_siz_x = DrawSize.x;
	float r_siz_y = DrawSize.y;

	int x_max = (int)r_siz_x + rad_x - 5;
	int y_max = (int)r_siz_y + rad_y - 5;

	screenx = rad_x + ((int)r_siz_x / 2 + int(x / range * r_siz_x));
	screeny = rad_y + ((int)r_siz_y / 2 + int(y / range * r_siz_y));

	if (screenx > x_max)
		screenx = x_max;

	if (screenx < rad_x)
		screenx = rad_x;

	if (screeny > y_max)
		screeny = y_max;

	if (screeny < rad_y)
		screeny = rad_y;
}

SDK::Structs::FVector* GetPawnRootLocations(uintptr_t pawn) {
	auto root = SDK::Utilities::read<uintptr_t>(pawn + SDK::Classes::StaticOffsets::RootComponent);
	if (!root) {
		return nullptr;
	}
	return reinterpret_cast<SDK::Structs::FVector*>(reinterpret_cast<PBYTE>(root) + SDK::Classes::StaticOffsets::RelativeLocation);

}

void renderRadar(uintptr_t CurrentActor, ImColor PlayerPointColor) {

	auto player = CurrentActor;

	int screenx = 0;
	int screeny = 0;

	SDK::Structs::FVector pos = *GetPawnRootLocations(CurrentActor);

	CalcRadarPoint(pos, screenx, screeny);

	ImDrawList* Draw = ImGui::GetOverlayDrawList();

	SDK::Structs::FVector viewPoint = { 0 };
	Draw->AddRectFilled(ImVec2((float)screenx, (float)screeny), ImVec2((float)screenx + 5, (float)screeny + 5), ImColor(PlayerPointColor));

}

inline bool custom_UseFontShadow;
inline unsigned int custom_FontShadowColor;

inline static void PushFontShadow(unsigned int col)
{
	custom_UseFontShadow = true;
	custom_FontShadowColor = col;
}

inline static void PopFontShadow(void)
{
	custom_UseFontShadow = false;
}

void RadarLoop(uintptr_t CurrentActor, ImColor PlayerPointColor)
{
	ImGuiWindowFlags TargetFlags;
	if (Settings.ShowMenu)
		TargetFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	else
		TargetFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	
	float radarWidth = 200;
	float PosDx = 1200;
	float PosDy = 60;

	if (!firstS) {
		ImGui::SetNextWindowPos(ImVec2{ 1200, 60 }, ImGuiCond_Once);
		firstS = true;
	}

	auto* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImColor(181, 181, 181);
	colors[ImGuiCol_ChildBg] = ImColor(181, 181, 181);
	colors[ImGuiCol_PopupBg] = ImColor(181, 181, 181);
	colors[ImGuiCol_Border] = ImColor(181, 181, 181);
	colors[ImGuiCol_BorderShadow] = ImColor(181, 181, 181);
	colors[ImGuiCol_TitleBg] = ImColor(181, 181, 181);
	colors[ImGuiCol_TitleBgActive] = ImColor(181, 181, 181);
	colors[ImGuiCol_TitleBgCollapsed] = ImColor(181, 181, 181);
	colors[ImGuiCol_MenuBarBg] = ImColor(181, 181, 181);


	if (ImGui::Begin("Radar", 0, ImVec2(200, 200), -1.f, TargetFlags)) {

		ImDrawList* Draw = ImGui::GetOverlayDrawList();
		ImVec2 DrawPos = ImGui::GetCursorScreenPos();
		ImVec2 DrawSize = ImGui::GetContentRegionAvail();

		ImVec2 midRadar = ImVec2(DrawPos.x + (DrawSize.x / 2), DrawPos.y + (DrawSize.y / 2));
		ImGui::GetWindowDrawList()->AddLine(ImVec2(midRadar.x - DrawSize.x / 2.f, midRadar.y), ImVec2(midRadar.x + DrawSize.x / 2.f, midRadar.y), IM_COL32(0, 0, 0, 90));
		ImGui::GetWindowDrawList()->AddLine(ImVec2(midRadar.x, midRadar.y - DrawSize.y / 2.f), ImVec2(midRadar.x, midRadar.y + DrawSize.y / 2.f), IM_COL32(0, 0, 0, 90));

		if (valid_pointer(PlayerController) && valid_pointer(PlayerCameraManager) && SDK::Utilities::CheckInScreen(CurrentActor, SDK::Utilities::CamLoc.x, SDK::Utilities::CamLoc.y)) { renderRadar(CurrentActor, PlayerPointColor); }

	}
	ImGui::End();
}

void DrawLine(int x1, int y1, int x2, int y2, ImColor color, int thickness)
{
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImColor(color)), thickness);
}

std::string TxtFormat(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	char buffer[2048];
	vsprintf(buffer, format, argptr);

	va_end(argptr);

	return buffer;
}

float width;
float height;
float X1 = GetSystemMetrics(0) / 2;
float Y1 = GetSystemMetrics(1) / 2;

#define ITEM_COLOR_MEDS ImVec4{ 0.9f, 0.55f, 0.55f, 0.95f }
#define ITEM_COLOR_SHIELDPOTION ImVec4{ 0.35f, 0.55f, 0.85f, 0.95f }
#define ITEM_COLOR_CHEST ImVec4{ 0.95f, 0.95f, 0.0f, 0.95f }
#define ITEM_COLOR_SUPPLYDROP ImVec4{ 0.9f, 0.1f, 0.1f, 0.9f }

inline float calc_distance(SDK::Structs::Vector3 camera_location, SDK::Structs::FVector pawn)
{
	float x = camera_location.x - pawn.X;
	float y = camera_location.y - pawn.Y;
	float z = camera_location.z - pawn.Z;
	float distance = sqrtf((x * x) + (y * y) + (z * z)) / 100.0f;
	return distance;
}

#define ReadPointer(base, offset) (*(PVOID *)(((PBYTE)base + offset)))
#define ReadDWORD(base, offset) (*(DWORD *)(((PBYTE)base + offset)))
#define ReadDWORD_PTR(base, offset) (*(DWORD_PTR *)(((PBYTE)base + offset)))
#define ReadFTRANSFORM(base, offset) (*(FTransform *)(((PBYTE)base + offset)))
#define ReadInt(base, offset) (*(int *)(((PBYTE)base + offset)))
#define ReadFloat(base, offset) (*(float *)(((PBYTE)base + offset)))
#define Readuintptr_t(base, offset) (*(uintptr_t *)(((PBYTE)base + offset)))
#define Readuint64_t(base, offset) (*(uint64_t *)(((PBYTE)base + offset)))
#define ReadVector3(base, offset) (*(Vector3 *)(((PBYTE)base + offset))

float ReloadNormal = 0.0f;
float ReloadTime = 0.0f;

float DrawOutlinedText(const std::string& text, const ImVec2& pos, ImU32 color, bool center)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	std::stringstream stream(text);
	std::string line;

	float y = 0.0f;
	int i = 0;

	while (std::getline(stream, line))
	{
		ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetOverlayDrawList()->_Data->FontSize, FLT_MAX, 0, line.c_str());


		if (center)
		{
			window->DrawList->AddText(ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			window->DrawList->AddText(ImVec2(pos.x - textSize.x / 2.0f, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}
		else
		{
			window->DrawList->AddText(ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(ImVec2((pos.x) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(ImVec2((pos.x) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			window->DrawList->AddText(ImVec2(pos.x, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}

		y = pos.y + textSize.y * (i + 1);
		i++;
	}
	return y;
}

bool distancegay = true;

#define ITEM_COLOR_TIER_WHITE ImVec4{ 0.8f, 0.8f, 0.8f, 0.95f }
#define ITEM_COLOR_TIER_GREEN ImVec4{ 0.0f, 0.95f, 0.0f, 0.95f }
#define ITEM_COLOR_TIER_BLUE ImVec4{ 0.2f, 0.4f, 1.0f, 0.95f }
#define ITEM_COLOR_TIER_PURPLE ImVec4{ 0.7f, 0.25f, 0.85f, 0.95f }
#define ITEM_COLOR_TIER_ORANGE ImVec4{ 0.85f, 0.65f, 0.0f, 0.95f }
#define ITEM_COLOR_TIER_GOLD ImVec4{ 0.95f, 0.85f, 0.45f, 0.95f }
#define ITEM_COLOR_TIER_UNKNOWN ImVec4{ 1.0f, 0.0f, 1.0f, 0.95f }

ImVec4 GetItemColor(BYTE tier)
{
	switch (tier)
	{
	case 1:
		return ITEM_COLOR_TIER_WHITE;
	case 2:
		return ITEM_COLOR_TIER_GREEN;
	case 3:
		return ITEM_COLOR_TIER_BLUE;
	case 4:
		return ITEM_COLOR_TIER_PURPLE;
	case 5:
		return ITEM_COLOR_TIER_ORANGE;
	case 6:
	case 7:
		return ITEM_COLOR_TIER_GOLD;
	case 8:
	case 9:
		return ImVec4{ 200 / 255.f, 0 / 255.f, 0 / 255.f, 0.95f };
	case 10:
		return ITEM_COLOR_TIER_UNKNOWN;
	default:
		return ITEM_COLOR_TIER_WHITE;
	}
}

int __forceinline GetDistanceMeters(SDK::Structs::Vector3 location, SDK::Structs::Vector3 CurrentActor)
{
	return (int)(location.Distance(CurrentActor) / 100);
}

bool ActorLoop(ImGuiWindow& window)
{
	uintptr_t MyTeamIndex;
	uintptr_t EnemyTeamIndex;
	float FOVmax = 9999.f;

	X = SDK::Utilities::SpoofCall(GetSystemMetrics, SM_CXSCREEN);
	Y = SDK::Utilities::SpoofCall(GetSystemMetrics, SM_CYSCREEN);

	uintptr_t GWorld = SDK::Utilities::read<uintptr_t>(Details.UWORLD);
	if (!GWorld) return false;

	uintptr_t Gameinstance = SDK::Utilities::read<uint64_t>(GWorld + SDK::Classes::StaticOffsets::OwningGameInstance);
	if (!Gameinstance) return false;

	uintptr_t LocalPlayer = SDK::Utilities::read<uint64_t>(Gameinstance + SDK::Classes::StaticOffsets::LocalPlayers);
	if (!LocalPlayer) return false;

	uintptr_t LocalPlayers = SDK::Utilities::read<uint64_t>(LocalPlayer);
	if (!LocalPlayers) return false;

	PlayerController = SDK::Utilities::read<uint64_t>(LocalPlayers + SDK::Classes::StaticOffsets::PlayerController);
	if (!PlayerController) return false;

	PlayerCameraManager = SDK::Utilities::read<uintptr_t>(PlayerController + SDK::Classes::StaticOffsets::PlayerCameraManager);
	if (!PlayerCameraManager) return false;

	LocalPawn = SDK::Utilities::read<uint64_t>(PlayerController + SDK::Classes::StaticOffsets::AcknowledgedPawn);
	//if (!LocalPawn) return false;

	uintptr_t Ulevel = SDK::Utilities::read<uintptr_t>(GWorld + SDK::Classes::StaticOffsets::PersistentLevel);
	if (!Ulevel) return false;

	uintptr_t AActors = SDK::Utilities::read<uintptr_t>(Ulevel + SDK::Classes::StaticOffsets::AActors);
	if (!AActors) return false;

	uintptr_t ActorCount = SDK::Utilities::read<int>(Ulevel + SDK::Classes::StaticOffsets::ActorCount);
	if (!ActorCount) return false;

	GetPlayerViewPoint(PlayerCameraManager, &SDK::Utilities::CamLoc, &SDK::Utilities::CamRot);

	for (int i = 0; i < ActorCount; i++) {

		uintptr_t CurrentActor = SDK::Utilities::read<uint64_t>(AActors + i * sizeof(uintptr_t));
		uintptr_t PlayerState = SDK::Utilities::read<uintptr_t>(CurrentActor + SDK::Classes::StaticOffsets::PlayerState);

		auto pawn = reinterpret_cast<SDK::Structs::UObject*>(SDK::Utilities::read<uint64_t>(AActors, i * sizeof(PVOID)));
		if (!pawn || pawn == (PVOID)LocalPawn) continue;

		std::string NameCurrentActor = GetObjectName(CurrentActor);
		std::string NameItemActor = GetObjectName((uintptr_t)pawn);

		if (strstr(NameCurrentActor.c_str(), xorthis("PlayerPawn_")) || strstr(NameCurrentActor.c_str(), xorthis("PlayerPawn_Athena_Phoebe_C")))
		{
			auto col = ImColor(218, 42, 28);

			auto SkelColor = ImColor(255, 255, 255);

			SDK::Structs::Vector3 headpos[3] = { 0 };
			SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 66, &headpos[3]);
			SDK::Classes::AController::WorldToScreen(SDK::Structs::Vector3(0), &headpos[3]);

			SDK::Structs::Vector3 Headbox, bottom, pelviss;

			SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 66, &Headbox);
			SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 2, &pelviss);
			SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 0, &bottom);

			SDK::Classes::AController::WorldToScreen(SDK::Structs::Vector3(Headbox.x, Headbox.y, Headbox.z + 20), &Headbox);
			SDK::Classes::AController::WorldToScreen(SDK::Structs::Vector3(pelviss.x, pelviss.y, pelviss.z + 70), &pelviss);
			SDK::Classes::AController::WorldToScreen(bottom, &bottom);

			if (Headbox.x == 0 && Headbox.y == 0) continue;
			if (bottom.x == 0 && bottom.y == 0) continue;

			float Height = Headbox.y - bottom.y;
			if (Height < 0)
				Height = Height * (-1.f);
			float Width = Height * 0.25;
			Headbox.x = Headbox.x - (Width / 2);

			float Height1 = Headbox.y - bottom.y;

			if (Height1 < 0)
				Height1 = Height1 * (-1.f);
			float Width1 = Height1 * 0.25;

			ImColor PlayerPointColor = { 0 };

			SDK::Structs::Vector3 head2, neck, pelvis, chest, leftShoulder, rightShoulder, leftElbow, rightElbow, leftHand, rightHand, leftLeg, rightLeg, leftThigh, rightThigh, leftFoot, rightFoot, leftFeet, rightFeet, leftFeetFinger, rightFeetFinger;

			if (valid_pointer(LocalPawn))
			{
				uintptr_t MyState = SDK::Utilities::read<uintptr_t>(LocalPawn + SDK::Classes::StaticOffsets::PlayerState);
				if (!MyState) continue;

				LocalWeapon = SDK::Utilities::read<uint64_t>(LocalPawn + SDK::Classes::StaticOffsets::CurrentWeapon);
				if (!LocalWeapon) continue;

				MyTeamIndex = SDK::Utilities::read<uintptr_t>(MyState + SDK::Classes::StaticOffsets::TeamIndex);
				if (!MyTeamIndex) continue;

				uintptr_t EnemyState = SDK::Utilities::read<uintptr_t>(CurrentActor + SDK::Classes::StaticOffsets::PlayerState);
				if (!EnemyState) continue;

				EnemyTeamIndex = SDK::Utilities::read<uintptr_t>(EnemyState + SDK::Classes::StaticOffsets::TeamIndex);
				if (!EnemyTeamIndex) continue;

				if (CurrentActor == LocalPawn) continue;

				SDK::Structs::Vector3 viewPoint;
				bool IsVisible;

				bool wasClicked = false;

// name esp here
				if (Settings.Visuals.PlayerName && SDK::Classes::Utils::CheckInScreen(CurrentActor, X, Y))
				{
					SDK::Structs::Vector3 headA;
					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 66, &headA);
					SDK::Classes::AController::WorldToScreen(headA, &headA);
					SDK::Structs::FString gayName = SDK::Utilities::read<SDK::Structs::FString>(EnemyState + 0x300);

					if (gayName.c_str())
					{
						CHAR Eichhörnchen[0xFF] = { 0 };
						wcstombs(Eichhörnchen, gayName.c_str(), sizeof(gayName));
						fn::general_overlay::OutlinedRBGText(headA.x, headA.y + 5, ImColor(161, 5, 5), TxtFormat(xorthis(" %s "), Eichhörnchen));
					}
				}

				if (Settings.VisibleCheck) {
					IsVisible = SDK::Classes::APlayerCameraManager::LineOfSightTo((PVOID)PlayerController, (PVOID)CurrentActor, &viewPoint);
					if (IsVisible) {
						col = ImColor((218, 42, 28));
						PlayerPointColor = ImColor((218, 42, 28));
					}
					else {
						col = ImColor((218, 42, 28));
						PlayerPointColor = ImColor((218, 42, 28));
					}
					if (IsVisible)
					{
						SkelColor = ImColor(155, 227, 0);
					}
					else
					{
						SkelColor = ImColor(155, 227, 0);
					}
				}

				if (SDK::Utilities::DiscordHelper::IsAiming())
				{
					if (SDK::Utilities::CheckIfInFOV(CurrentActor, FOVmax)) {

						if (Settings.VisibleCheck and IsVisible)
						{
							if (Settings.Aimbot.Memory)
							{
								SDK::Structs::Vector3 NewAngle = SDK::Utilities::GetRotation(CurrentActor);

								if (NewAngle.x == 0 && NewAngle.y == 0) continue;

								if (Settings.Aimbot.Smoothness > 0)
									NewAngle = SDK::Utilities::SmoothAngles(SDK::Utilities::CamRot, NewAngle);

								NewAngle.z = 0;

								SDK::Classes::AController::ValidateClientSetRotation(NewAngle, false);
								SDK::Classes::AController::ClientSetRotation(NewAngle, false);
							}
						}
						else if (!Settings.VisibleCheck)
						{
							SDK::Structs::Vector3 NewAngle = SDK::Utilities::GetRotation(CurrentActor);

							if (NewAngle.x == 0 && NewAngle.y == 0) continue;

							if (Settings.Aimbot.Smoothness > 0)
								NewAngle = SDK::Utilities::SmoothAngles(SDK::Utilities::CamRot, NewAngle);

							NewAngle.z = 0;

							SDK::Classes::AController::ValidateClientSetRotation(NewAngle, false);
							SDK::Classes::AController::ClientSetRotation(NewAngle, false);

						}
					}
				}
			}

			if (Settings.Visuals.Box) {
				if (Settings.Visuals.BoxTypePos == 0)
				{
					{
						fn::general_overlay::Rect(Headbox.x, Headbox.y, Width, Height, ImColor(0, 0, 0, 200), 3.5);
						fn::general_overlay::Rect(Headbox.x, Headbox.y, Width, Height, ImColor(col), 2);
					}
				}

				else if (Settings.Visuals.BoxTypePos == 1) {
					fn::general_overlay::DrawCorneredBox(Headbox.x, Headbox.y, Width, Height, ImColor(0, 0, 0, 200), 3.5);
					fn::general_overlay::DrawCorneredBox(Headbox.x, Headbox.y, Width, Height, ImColor(col), 2);
				}

				else if (Settings.Visuals.BoxTypePos == 2)
				{
					fn::general_overlay::FilledRect(Headbox.x, Headbox.y, Width, Height, ImColor(0, 0, 0, 50));
					fn::general_overlay::Rect(Headbox.x, Headbox.y, Width, Height, ImColor(0, 0, 0, 200), 2);
				}
			}

			if (Settings.Visuals.DrawRadar) {
				RadarLoop(CurrentActor, ImColor(237, 28, 36));
			}

			//Exploits
			
			

			if (Settings.Exploits.NoBloom)
			{
				uintptr_t CurrentWeapon = SDK::Utilities::read<uintptr_t>(LocalPawn + 0x610);
				if (CurrentWeapon) {
					SDK::Utilities::write<uintptr_t>(CurrentWeapon + SDK::Classes::StaticOffsets::PlayerController, 0.33f);
				}
			}

			if (Settings.Exploits.FastReload) {
				uintptr_t CurrentWeapon = SDK::Utilities::read<uintptr_t>(LocalPawn + 0x610);
				float a = SDK::Utilities::read<float>(CurrentWeapon + SDK::Classes::StaticOffsets::LastReloadTime);
				float b = SDK::Utilities::read<float>(CurrentWeapon + SDK::Classes::StaticOffsets::LastSuccessfulReloadTime);
				SDK::Utilities::write<float>(CurrentWeapon + SDK::Classes::StaticOffsets::LastReloadTime, a + b - 1000); //RapidFire value
			}
			if (Settings.Exploits.RapidFire) {
				uintptr_t CurrentWeapon = SDK::Utilities::read<uintptr_t>(LocalPawn + 0x610);
				if (CurrentWeapon) {
					float a = SDK::Utilities::read<float>(CurrentWeapon + SDK::Classes::StaticOffsets::LastFireTime);
					float b = SDK::Utilities::read<float>(CurrentWeapon + SDK::Classes::StaticOffsets::LastFireTimeVerified);
					SDK::Utilities::write<float>(CurrentWeapon + SDK::Classes::StaticOffsets::LastFireTime, (a + b - 1000));
				}
			}

			if (Settings.Exploits.AimWhileJumping) {
				SDK::Utilities::write<bool>(LocalPawn + SDK::Classes::StaticOffsets::bADSWhileNotOnGround, true);
			}
			else {
				SDK::Utilities::write<bool>(LocalPawn + SDK::Classes::StaticOffsets::bADSWhileNotOnGround, false);
			}

			if (Settings.Exploits.InstantRevive && GetAsyncKeyState(0x45)) {
				SDK::Utilities::write<float>(LocalPawn + SDK::Classes::StaticOffsets::ReviveFromDBNOTime, 0.101);
			}

			if (Settings.Visuals.Snaplines && SDK::Classes::Utils::CheckInScreen(CurrentActor, X, Y)) {
				fn::general_overlay::Outline(X / 2, Y, bottom.x, bottom.y, ImColor(188, 26, 66));
				fn::general_overlay::DrawLine(X / 2, Y, bottom.x, bottom.y, ImColor(188, 26, 66), 1.5);
			}



			
			if (Settings.Visuals.Skeleton)
			{
				SDK::Structs::Vector3 head2, neck, pelvis, chest, leftShoulder, rightShoulder, leftElbow, rightElbow, leftHand, rightHand, leftLeg, rightLeg, leftThigh, rightThigh, leftFoot, rightFoot, leftFeet, rightFeet, leftFeetFinger, rightFeetFinger;

				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 66, &head2);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 65, &neck);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 2, &pelvis);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 36, &chest);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 9, &leftShoulder);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 62, &rightShoulder);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 10, &leftElbow);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 38, &rightElbow);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 11, &leftHand);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 39, &rightHand);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 67, &leftLeg);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 74, &rightLeg);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 73, &leftThigh);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 80, &rightThigh);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &leftFoot);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 75, &rightFoot);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 71, &leftFeet);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 78, &rightFeet);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 72, &leftFeetFinger);
				SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 79, &rightFeetFinger);

				SDK::Classes::AController::WorldToScreen(head2, &head2);
				SDK::Classes::AController::WorldToScreen(neck, &neck);
				SDK::Classes::AController::WorldToScreen(pelvis, &pelvis);
				SDK::Classes::AController::WorldToScreen(chest, &chest);
				SDK::Classes::AController::WorldToScreen(leftShoulder, &leftShoulder);
				SDK::Classes::AController::WorldToScreen(rightShoulder, &rightShoulder);
				SDK::Classes::AController::WorldToScreen(leftElbow, &leftElbow);
				SDK::Classes::AController::WorldToScreen(rightElbow, &rightElbow);
				SDK::Classes::AController::WorldToScreen(leftHand, &leftHand);
				SDK::Classes::AController::WorldToScreen(rightHand, &rightHand);
				SDK::Classes::AController::WorldToScreen(leftLeg, &leftLeg);
				SDK::Classes::AController::WorldToScreen(rightLeg, &rightLeg);
				SDK::Classes::AController::WorldToScreen(leftThigh, &leftThigh);
				SDK::Classes::AController::WorldToScreen(rightThigh, &rightThigh);
				SDK::Classes::AController::WorldToScreen(leftFoot, &leftFoot);
				SDK::Classes::AController::WorldToScreen(rightFoot, &rightFoot);
				SDK::Classes::AController::WorldToScreen(leftFeet, &leftFeet);
				SDK::Classes::AController::WorldToScreen(rightFeet, &rightFeet);
				SDK::Classes::AController::WorldToScreen(leftFeetFinger, &leftFeetFinger);
				SDK::Classes::AController::WorldToScreen(rightFeetFinger, &rightFeetFinger);
				fn::general_overlay::DrawLine(head2.x, head2.y, neck.x, neck.y, SkelColor, 1);
				fn::general_overlay::DrawLine(neck.x, neck.y, pelvis.x, pelvis.y, SkelColor, 1);
				fn::general_overlay::DrawLine(chest.x, chest.y, leftShoulder.x, leftShoulder.y, SkelColor, 1);
				fn::general_overlay::DrawLine(chest.x, chest.y, rightShoulder.x, rightShoulder.y, SkelColor, 1);
				fn::general_overlay::DrawLine(leftShoulder.x, leftShoulder.y, leftElbow.x, leftElbow.y, SkelColor, 1);
				fn::general_overlay::DrawLine(rightShoulder.x, rightShoulder.y, rightElbow.x, rightElbow.y, SkelColor, 1);
				fn::general_overlay::DrawLine(leftElbow.x, leftElbow.y, leftHand.x, leftHand.y, SkelColor, 1);
				fn::general_overlay::DrawLine(rightElbow.x, rightElbow.y, rightHand.x, rightHand.y, SkelColor, 1);
				fn::general_overlay::DrawLine(pelvis.x, pelvis.y, leftLeg.x, leftLeg.y, SkelColor, 1);
				fn::general_overlay::DrawLine(pelvis.x, pelvis.y, rightLeg.x, rightLeg.y, SkelColor, 1);
				fn::general_overlay::DrawLine(leftLeg.x, leftLeg.y, leftThigh.x, leftThigh.y, SkelColor, 1);
				fn::general_overlay::DrawLine(rightLeg.x, rightLeg.y, rightThigh.x, rightThigh.y, SkelColor, 1);
				fn::general_overlay::DrawLine(leftThigh.x, leftThigh.y, leftFoot.x, leftFoot.y, SkelColor, 1);
				fn::general_overlay::DrawLine(rightThigh.x, rightThigh.y, rightFoot.x, rightFoot.y, SkelColor, 1);
				fn::general_overlay::DrawLine(leftFoot.x, leftFoot.y, leftFeet.x, leftFeet.y, SkelColor, 1);
				fn::general_overlay::DrawLine(rightFoot.x, rightFoot.y, rightFeet.x, rightFeet.y, SkelColor, 1);
				fn::general_overlay::DrawLine(leftFeet.x, leftFeet.y, leftFeetFinger.x, leftFeetFinger.y, SkelColor, 1);
				fn::general_overlay::DrawLine(rightFeet.x, rightFeet.y, rightFeetFinger.x, rightFeetFinger.y, SkelColor, 1);

			}
		}
	}

	return true;
}

std::string random_string(std::size_t length)
{
	const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	std::random_device random_device;
	std::mt19937 generator(random_device());
	std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

	std::string random_string;

	for (std::size_t i = 0; i < length; ++i)
	{
		random_string += CHARACTERS[distribution(generator)];
	}

	return random_string;
}

void RenderMenu()
{
	float ScreenCenterX = X1;
	float ScreenCenterY = Y1;

	if (Settings.Visuals.DrawCrosshair)
	{
		//fn::general_overlay::DrawLine(ScreenCenterX / 2, ScreenCenterY / 2, X1, Y1, ImColor(0, 0, 0), 3.f);
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(ScreenCenterX - 8.f, ScreenCenterY), ImVec2((ScreenCenterX - 8.f) + (8.f * 2), ScreenCenterY), ImColor(255, 0, 0));
		ImGui::GetOverlayDrawList()->AddLine(ImVec2(ScreenCenterX, ScreenCenterY - 8.f), ImVec2(ScreenCenterX, (ScreenCenterY - 8.f) + (8.f * 2)), ImColor(255, 0, 0));
	}

	if (Settings.Visuals.DrawFOV)
	{
		ImGui::GetOverlayDrawList()->AddCircle(ImVec2(X1, Y1), Settings.Aimbot.FOV, ImColor(255, 255, 255), 35);
		ImGui::GetOverlayDrawList()->AddCircle(ImVec2(X1, Y1), Settings.Aimbot.FOV + 0.5f, ImColor(255, 255, 255), 35);
	}

	if (Settings.ShowMenu)
	{
		auto* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(1.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.41f, 0.00f, 0.03f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.48f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(4.00f, 3.00f);
		style.ItemSpacing = ImVec2(8.00f, 4.00f);
		style.ItemInnerSpacing = ImVec2(4.00f, 4.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.IndentSpacing = 21.00f;
		style.ScrollbarSize = 14.00f;
		style.GrabMinSize = 10.00f;
		style.WindowBorderSize = 0.00f;
		style.ChildBorderSize = 0.00f;
		style.PopupBorderSize = 1.00f;
		style.FrameBorderSize = 1.00f;
		style.WindowRounding = 0.00f;
		style.ChildRounding = 0.00f;
		style.FrameRounding = 6.00f;
		style.PopupRounding = 0.00f;
		style.ScrollbarRounding = 9.00f;
		style.GrabRounding = 7.00f;
		style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
		style.DisplaySafeAreaPadding = ImVec2(3.00f, 3.00f);

		ImGui::Begin("LFG`s Internal", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		ImGui::Tab(0, "Aimbot", &Settings.MenuTab, 190);
		ImGui::Tab(1, "Visuals", &Settings.MenuTab, 190);
		ImGui::Tab(2, "Exploits", &Settings.MenuTab, 190);
		ImGui::Tab(3, "Misc", &Settings.MenuTab, 190);

		if (Settings.MenuTab == 0)
		{
			ImGui::Columns(1, NULL, false);

			ImGui::Text("Aimbot");
			ImGui::BeginChild("##aimbot_main_child", ImVec2(390, 400), true);

			ImGui::Checkbox("Memory Aimbot", &Settings.Aimbot.Memory);
			ImGui::Checkbox("Silent Aimbot", &Settings.Aimbot.Silent);
			ImGui::Checkbox("Visible Check", &Settings.VisibleCheck);
			ImGui::Checkbox("Aim Holdover", &Settings.Aimbot.Holdover);
			ImGui::Combo("Aim Bone", &Settings.Aimbot.BonePos, AimBones, sizeof(AimBones) / sizeof(*AimBones));
			ImGui::SliderFloat("Aim FOV", &Settings.Aimbot.FOV, 0, 1000);
			ImGui::SliderFloat("Aim Smoothing", &Settings.Aimbot.Smoothness, 1, 20);

			ImGui::EndChild();
		}

		else if (Settings.MenuTab == 1)
		{
			ImGui::Columns(3, NULL, false);

			ImGui::Text("Player ESP");
			ImGui::BeginChild("player_esp_child", ImVec2(260, 400), true);

			ImGui::Checkbox("Box", &Settings.Visuals.Box);
			ImGui::Combo("Box Type", &Settings.Visuals.BoxTypePos, BoxTypes, sizeof(BoxTypes) / sizeof(*BoxTypes));
			ImGui::Checkbox("Name", &Settings.Visuals.PlayerName);
			ImGui::Checkbox("Active Item", &Settings.Visuals.PlayerActiveItem);
			ImGui::Checkbox("Skeleton", &Settings.Visuals.Skeleton);
			ImGui::Checkbox("Snaplines", &Settings.Visuals.Snaplines);

			ImGui::EndChild();
			ImGui::NextColumn();

			ImGui::Text("World ESP");
			ImGui::BeginChild("world_esp_child", ImVec2(260, 400), true);

			ImGui::EndChild();
			ImGui::NextColumn();

			ImGui::Text("Misc");
			ImGui::BeginChild("misc_esp_child", ImVec2(260, 400), true);

			ImGui::Checkbox("Draw FOV", &Settings.Visuals.DrawFOV);
			ImGui::Checkbox("Draw Crosshair", &Settings.Visuals.DrawCrosshair);
			ImGui::Checkbox("Draw Radar", &Settings.Visuals.DrawRadar);
			ImGui::SliderFloat("ESP Distance", &Settings.Visuals.MaxDistance, 10, 1000);

			ImGui::EndChild();
		}

		else if (Settings.MenuTab == 2)
		{
			ImGui::Columns(1, NULL, false);

			ImGui::Text("Exploits");
			ImGui::BeginChild("##exploits_options_child", ImVec2(390, 400), true);

			ImGui::Checkbox("No Spread", &Settings.Exploits.NoSpread);
			ImGui::Checkbox("No Bloom", &Settings.Exploits.NoBloom);
			ImGui::Checkbox("Fast Reload", &Settings.Exploits.FastReload);
			ImGui::Checkbox("Rapid Fire", &Settings.Exploits.RapidFire);
			ImGui::Checkbox("Aim While Jumping", &Settings.Exploits.AimWhileJumping);
			ImGui::Checkbox("Instant Revive", &Settings.Exploits.InstantRevive);
			ImGui::Checkbox("Bullet Teleport", &Settings.Exploits.BulletTeleport);
			ImGui::Checkbox("Camera FOV Changer", &Settings.Exploits.CameraFOVChanger);

			ImGui::EndChild();
		}

		else if (Settings.MenuTab == 3)
		{
			ImGui::Columns(1, NULL, false);

			ImGui::Text("Misc Options");
			ImGui::BeginChild("##misc_options_child", ImVec2(390, 400), true);

			ImGui::Checkbox("Visibility Check", &Settings.VisibleCheck);

			ImGui::EndChild();
		}

		ImGui::End();
	}

	MenuKey();
}
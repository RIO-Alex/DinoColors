#include <fstream>
#include <API/ARK/Ark.h>
#include "json.hpp"

#pragma comment(lib, "AsaApi.lib")

DECLARE_HOOK(APrimalDinoCharacter_BeginPlay, void, APrimalDinoCharacter*);

nlohmann::json config;

int GetRandomNumber(int min, int max)
{
	const int n = max - min + 1;
	const int remainder = RAND_MAX % n;
	int x;

	do
	{
		x = rand();
	} while (x >= RAND_MAX - remainder);

	return min + x % n;
}

int GetRandomColor()
{
	auto all_colors = config["Colors"];

	const size_t size = all_colors.size() - 1;
	const int rnd = GetRandomNumber(0, static_cast<int>(size));

	return all_colors[rnd];
}

void RandomizeDinoColor(APrimalDinoCharacter* dino)
{
	for (int i = 0; i <= 5; ++i)
	{
		const int color = GetRandomColor();

		dino->ForceUpdateColorSets_Implementation(i, color);
	}
}

void Hook_APrimalDinoCharacter_BeginPlay(APrimalDinoCharacter* _this)
{
	APrimalDinoCharacter_BeginPlay_original(_this);

	// Don't change color of tamed dinos
	if (_this->TargetingTeamField() < 50000)
	{
		RandomizeDinoColor(_this);
	}
}

void ReadConfig()
{
	const std::string config_path = AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DinoColors/colors.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
		throw std::runtime_error("Can't open colors.json");

	file >> config;

	file.close();
}


void Load()
{
	Log::Get().Init("DinoColors");
	
	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	srand(time(nullptr));

	AsaApi::GetHooks().SetHook("APrimalDinoCharacter.BeginPlay()", &Hook_APrimalDinoCharacter_BeginPlay, &APrimalDinoCharacter_BeginPlay_original);
}

void Unload()
{
	AsaApi::GetHooks().DisableHook("APrimalDinoCharacter.BeginPlay()", &Hook_APrimalDinoCharacter_BeginPlay);
}

BOOL APIENTRY DllMain(HMODULE, DWORD ul_reason_for_call, LPVOID)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}
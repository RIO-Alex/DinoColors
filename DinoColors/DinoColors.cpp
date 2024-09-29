#include <fstream>
#include <API/ARK/Ark.h>
#include "json.hpp"

#pragma comment(lib, "AsaApi.lib")

DECLARE_HOOK(APrimalDinoCharacter_BeginPlay, void, APrimalDinoCharacter*);

nlohmann::json config;

void SendRconReply(RCONClientConnection* rcon_connection, int packet_id, const FString& msg)
{
	FString reply = msg + "\n";
	rcon_connection->SendMessageW(packet_id, 0, &reply);
}

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
	if (_this->TargetingTeamField() < 50000 && config.value("EnableColors", true))
	{
		RandomizeDinoColor(_this);
	}
}

void ReadConfig()
{
	const std::string DefaultConfigPath = AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DinoColors/config.json";
	std::ifstream file{ DefaultConfigPath };
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();

	std::string OverrideConfigPath = config.value("ConfigPathOverride", "");
	if (!OverrideConfigPath.empty())
	{
		std::ifstream OverrideConfig{ OverrideConfigPath };
		if (!OverrideConfig.is_open())
			throw std::runtime_error("Can't open config.json from ConfigPathOverride parameter.");

		OverrideConfig >> config;

		OverrideConfig.close();
	}
}

void ReloadConfig_RCON(RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*)
{
	FString rep = L"Failed to reload config";

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		SendRconReply(rcon_connection, rcon_packet->Id, rep);
		Log::GetLog()->error(error.what());
		return;
	}

	rep = L"Reloaded config";
	SendRconReply(rcon_connection, rcon_packet->Id, rep);
}

void ReloadConfig_Cmd(APlayerController* player, FString* Command, bool)
{
	FString rep = L"Failed to reload config";

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		AsaApi::GetApiUtils().SendServerMessage(static_cast<AShooterPlayerController*>(player), FColorList::Green, rep.ToString().c_str());
		Log::GetLog()->error(error.what());
		return;
	}

	rep = L"Reloaded config";
	AsaApi::GetApiUtils().SendServerMessage(static_cast<AShooterPlayerController*>(player), FColorList::Green, rep.ToString().c_str());
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
	AsaApi::GetCommands().AddRconCommand("dc.reload", &ReloadConfig_RCON);
	AsaApi::GetCommands().AddConsoleCommand("DCReloadConfig", &ReloadConfig_Cmd);
}

void Unload()
{
	AsaApi::GetHooks().DisableHook("APrimalDinoCharacter.BeginPlay()", &Hook_APrimalDinoCharacter_BeginPlay);
	AsaApi::GetCommands().RemoveRconCommand("dc.reload");
	AsaApi::GetCommands().RemoveConsoleCommand("DCReloadConfig");
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
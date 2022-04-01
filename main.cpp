
#include "memory.h" //Helper function for accessing memory to the game
#include "vector.h" //Helper functions to calculate vectors for aimbot
#include <thread>

//List of offsets where the memory of certain function of the game are initialised for the program
namespace offset {

	//client
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDB65DC;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4DD245C;
	constexpr ::std::ptrdiff_t dwGlowObjectManager = 0x531B048;

	//Engine
	constexpr ::std::ptrdiff_t dwClientState = 0x58CFC4;
	constexpr ::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;
	constexpr ::std::ptrdiff_t dwClientState_GetLocalPlayer = 0x180;

	//Entity
	constexpr ::std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
	constexpr ::std::ptrdiff_t m_bDormant = 0xED;
	constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr ::std::ptrdiff_t m_lifeState = 0x25F;
	constexpr ::std::ptrdiff_t m_vecOrigin = 0x138;
	constexpr ::std::ptrdiff_t m_vecViewOffset = 0x108;
	constexpr ::std::ptrdiff_t m_aimPunchAngle = 0x303C;
	constexpr ::std::ptrdiff_t m_bSpottedByMask = 0x980;
	constexpr ::std::ptrdiff_t m_iGlowIndex = 0x10488;


}

//Function the vector between the player and enemy and calculate the angle between both parties
constexpr vector3 CalculateAngle(const vector3& localPosition, const vector3& enemyPosition, const vector3& viewAngles) {
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

int main() {
	//Initialise memory
	const auto memory = Memory{ "csgo.exe" };
	
	//Module addresses
	const auto client = memory.GetModuleAddress("client.dll");
	const auto engine = memory.GetModuleAddress("engine.dll");



	//infinite aimbot loop
	while (true) {

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//Accessing local player position
		const auto localPlayer = memory.Read<std::uintptr_t>(client + offset::dwLocalPlayer);

		//Accessing the glow function in the game files
		const auto glowObjectManager = memory.Read<std::uintptr_t>(client + offset::dwGlowObjectManager);

		//Accessing local team position
		const auto localTeam = memory.Read<std::int32_t>(localPlayer + offset::m_iTeamNum);

		//Colour the enemies
		for (auto i = 1; i <= 32; ++i) {
			const auto player = memory.Read<std::uintptr_t>(client + offset::dwEntityList + i * 0x10);
			
			//Is the subject a teammate? If no, then highlight them
			if (memory.Read<std::int32_t>(player + offset::m_iTeamNum) == localTeam) {
				continue;
			}


			const auto glowIndex = memory.Read<std::int32_t>(player + offset::m_iGlowIndex);
			//Colour the enemies red
			memory.Write<float>(glowObjectManager + (glowIndex * 0x38) + 0x8, 1.f); //r

			//Colour the enemies green
			memory.Write<float>(glowObjectManager + (glowIndex * 0x38) + 0xC, 0.f); //g

			//Colour the enemies blue
			memory.Write<float>(glowObjectManager + (glowIndex * 0x38) + 0x10, 0.f); //b

			//Change the opacity of the colour
			memory.Write<float>(glowObjectManager + (glowIndex * 0x38) + 0x14, 1.f); //a

			memory.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x27, true);
			memory.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x28, true);

		}

		//Keybind for aimbot, currently right mouse button
		if (!GetAsyncKeyState(VK_MBUTTON)) {
			continue;
		}

		//local eye position = Origin + viewoffset
		const auto localEyePosition = memory.Read<vector3>(localPlayer + offset::m_vecOrigin) + memory.Read<vector3>(localPlayer + offset::m_vecViewOffset);

		const auto clientState = memory.Read<std::uintptr_t>(engine + offset::dwClientState);

		const auto localPlayerId = memory.Read<std::int32_t>(clientState + offset::dwClientState_GetLocalPlayer);

		const auto viewAngles = memory.Read<vector3>(clientState + offset::dwClientState_ViewAngles);

		const auto aimPunch = memory.Read<vector3>(localPlayer + offset::m_aimPunchAngle) * 2;

		//Aimbot Field of View (FOV)
		auto bestFov = 5.f;
		auto bestAngle = vector3{};

		//Loop through a criteria to determine if the subject being targeted is an enemy. Loop 64 times since that is the max possible number of players
		for (auto i = 1; i <= 32; ++i) {

			//Read memory of player
			const auto player = memory.Read<std::uintptr_t>(client + offset::dwEntityList + i * 0x10);

			//Is the subject a teammate. If they are, do not shoot them
			if (memory.Read<std::int32_t>(player + offset::m_iTeamNum) == localTeam) {
				continue;
			}

			//Is a object or a player
			if (memory.Read<bool>(player + offset::m_bDormant)) {
				continue;
			}

			//Is the subject alive or dead. If dead, do not shoot them
			if (memory.Read<std::int32_t>(player + offset::m_lifeState)) {
				continue;
			}

			//Is the subject spotted by the player
			if (!memory.Read<bool>(player + offset::m_bSpottedByMask) && (1<<localPlayerId)) {
				continue;
			}

			//3d structure of the player model
			const auto boneMatrix = memory.Read<std::uintptr_t>(player + offset::m_dwBoneMatrix);

			//Position of player's head in 3d space
			const auto playerHeadPosition = vector3{ memory.Read<float>(boneMatrix + 0x30 * 8 + 0x0C), 
													 memory.Read<float>(boneMatrix + 0x30 * 8 + 0x1C),
													 memory.Read<float>(boneMatrix + 0x30 * 8 + 0x2C) };

			const auto angle = CalculateAngle(localEyePosition, playerHeadPosition, viewAngles + aimPunch);

			const auto fov = std::hypot(angle.x, angle.y);

			if (fov < bestFov) {
				bestFov = fov;
				bestAngle = angle;
			}

		}
		
		//Once the enemy has been identified with using the criteria above, initiate the aimbot.
		//Note: change the number at the end of the memory.write line to change the smoothness of the aimbot. The lower the number, the faster the aimbot.
		if (!bestAngle.IsZero()) {
			memory.Write<vector3>(clientState + offset::dwClientState_ViewAngles, viewAngles + bestAngle / 1.f);
			mouse_event(MOUSEEVENTF_LEFTDOWN,NULL,NULL,0,0);
			mouse_event(MOUSEEVENTF_LEFTUP, NULL, NULL, 0, 0);
		}
	}
	return 0;
}

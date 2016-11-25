#pragma once

int wardenModuleAddress;

int WardenScanDetour(int ptr, int address, int len);

int SMSG_WARDEN_DATA_HandlerDetour(int a1, uint16 opcode, int a3, int pDataStore)
{
	if (opcode == SMSG_WARDEN_DATA)
	{
		if (*(int*)0x00D31A48 != 0)
		{
			int vtable = *(int*)(*(int*)0x00D31A4C);
			int wardenMain = *(int*)(vtable + 8);
			int wardenModule = wardenMain - 0x4099;

			if ((wardenModule & 0xFF) == 0 && wardenModuleAddress != wardenModule)
			{
				wardenModuleAddress = wardenModule;
				
				//bypass PAGE_CHECK_A, PAGE_CHECK_B, DRIVER_CHECK and MODULE_CHECK
				DWORD oldPFlags;
				VirtualProtect((void*)(wardenModule + 0x309F), 1, 0x40, &oldPFlags);
				*(byte*)(wardenModule + 0x309F) = 0xE9;
				VirtualProtect((void*)(wardenModule + 0x33D1), 1, 0x40, &oldPFlags);
				*(byte*)(wardenModule + 0x33D1) = 0;

				auto det = g_Detours["WardenScanDetour"];
				if (det != nullptr)
					delete det;

				g_Detours["WardenScanDetour"] = new Detour((wardenModule + 0x2A7F), (int)WardenScanDetour);
			}
		}
	}

	//---------------- return to the original function ----------------
	auto det = g_Detours["WardenDataHandler"];
	det->Restore();
	int res = ((int(__cdecl*)(int, uint16, int, int))det->GetOrig())(a1, opcode, a3, pDataStore);
	det->Apply();
	return res;
}

int WardenScanDetour(int ptr, int address, int len)
{
	std::map<int, byte> orig_bytes{};

	for (auto& det : g_Detours)
	{
		for (int i = 0; i != 6; ++i)
		{
			orig_bytes[(int)(det.second->target_func) + i] = *(det.second->original_bytes + i);
		}
	}
	
	for (int i = 0; i != len; ++i)
	{
		
		if (orig_bytes.find(address + i) == orig_bytes.end()) 
			*(byte*)(ptr + i) = *(byte*)(address + i);		
		else 
			//bypass MEM_CHECK for detoured functions
			*(byte*)(ptr + i) = orig_bytes[address + i];
	}

	return 0;
}
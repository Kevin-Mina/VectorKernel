#include <ntifs.h>

#define DRIVER_PREFIX "InjectLibraryDrv: "
#define DEVICE_PATH L"\\Device\\InjectLibrary"
#define SYMLINK_PATH L"\\??\\InjectLibrary"
#define DRIVER_TAG 'lnKV'

#pragma warning(disable: 4201) // This warning is caused by SECTION_IMAGE_INFORMATION definition

//
// Ioctl code definition
//
#define IOCTL_INJECT_LIBRARY CTL_CODE(0x8000, 0x0600, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Custom sturct definition
//
typedef struct _INJECT_CONTEXT
{
	ULONG ThreadId;
	WCHAR LibraryPath[256];
} INJECT_CONTEXT, *PINJECT_CONTEXT;

//
// Windows definition
//
typedef struct _IMAGE_DOS_HEADER
{
	USHORT e_magic;
	USHORT e_cblp;
	USHORT e_cp;
	USHORT e_crlc;
	USHORT e_cparhdr;
	USHORT e_minalloc;
	USHORT e_maxalloc;
	USHORT e_ss;
	USHORT e_sp;
	USHORT e_csum;
	USHORT e_ip;
	USHORT e_cs;
	USHORT e_lfarlc;
	USHORT e_ovno;
	USHORT e_res[4];
	USHORT e_oemid;
	USHORT e_oeminfo;
	USHORT e_res2[10];
	LONG e_lfanew;
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER
{
	SHORT Machine;
	SHORT NumberOfSections;
	LONG TimeDateStamp;
	LONG PointerToSymbolTable;
	LONG NumberOfSymbols;
	SHORT SizeOfOptionalHeader;
	SHORT Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY
{
	LONG VirtualAddress;
	LONG Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER64
{
	SHORT Magic;
	UCHAR MajorLinkerVersion;
	UCHAR MinorLinkerVersion;
	LONG SizeOfCode;
	LONG SizeOfInitializedData;
	LONG SizeOfUninitializedData;
	LONG AddressOfEntryPoint;
	LONG BaseOfCode;
	ULONG64 ImageBase;
	LONG SectionAlignment;
	LONG FileAlignment;
	SHORT MajorOperatingSystemVersion;
	SHORT MinorOperatingSystemVersion;
	SHORT MajorImageVersion;
	SHORT MinorImageVersion;
	SHORT MajorSubsystemVersion;
	SHORT MinorSubsystemVersion;
	LONG Win32VersionValue;
	LONG SizeOfImage;
	LONG SizeOfHeaders;
	LONG CheckSum;
	SHORT Subsystem;
	SHORT DllCharacteristics;
	ULONG64 SizeOfStackReserve;
	ULONG64 SizeOfStackCommit;
	ULONG64 SizeOfHeapReserve;
	ULONG64 SizeOfHeapCommit;
	LONG LoaderFlags;
	LONG NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS64
{
	LONG Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

typedef struct _IMAGE_EXPORT_DIRECTORY
{
	ULONG Characteristics;
	ULONG TimeDateStamp;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG Name;
	ULONG Base;
	ULONG NumberOfFunctions;
	ULONG NumberOfNames;
	ULONG AddressOfFunctions;
	ULONG AddressOfNames;
	ULONG AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _SECTION_IMAGE_INFORMATION
{
	PVOID TransferAddress;
	ULONG ZeroBits;
	SIZE_T MaximumStackSize;
	SIZE_T CommittedStackSize;
	ULONG SubSystemType;
	union
	{
		struct
		{
			USHORT SubSystemMinorVersion;
			USHORT SubSystemMajorVersion;
		};
		ULONG SubSystemVersion;
	};
	union
	{
		struct
		{
			USHORT MajorOperatingSystemVersion;
			USHORT MinorOperatingSystemVersion;
		};
		ULONG OperatingSystemVersion;
	};
	USHORT ImageCharacteristics;
	USHORT DllCharacteristics;
	USHORT Machine;
	BOOLEAN ImageContainsCode;
	union
	{
		UCHAR ImageFlags;
		struct
		{
			UCHAR ComPlusNativeReady : 1;
			UCHAR ComPlusILOnly : 1;
			UCHAR ImageDynamicallyRelocated : 1;
			UCHAR ImageMappedFlat : 1;
			UCHAR BaseBelow4gb : 1;
			UCHAR ComPlusPrefer32bit : 1;
			UCHAR Reserved : 2;
		};
	};
	ULONG LoaderFlags;
	ULONG ImageFileSize;
	ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

//
// enum definition
//
typedef enum _SECTION_INFORMATION_CLASS
{
	SectionBasicInformation, // q; SECTION_BASIC_INFORMATION
	SectionImageInformation, // q; SECTION_IMAGE_INFORMATION
	SectionRelocationInformation, // q; ULONG_PTR RelocationDelta // name:wow64:whNtQuerySection_SectionRelocationInformation // since WIN7
	SectionOriginalBaseInformation, // q; PVOID BaseAddress // since REDSTONE
	SectionInternalImageInformation, // SECTION_INTERNAL_IMAGE_INFORMATION // since REDSTONE2
	MaxSectionInfoClass
} SECTION_INFORMATION_CLASS;

typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment
} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

//
// Function type definition
//
typedef NTSTATUS(NTAPI *PZwQuerySection)(
	_In_ HANDLE SectionHandle,
	_In_ SECTION_INFORMATION_CLASS SectionInformationClass,
	_Out_writes_bytes_(SectionInformationLength) PVOID SectionInformation,
	_In_ SIZE_T SectionInformationLength,
	_Out_opt_ PSIZE_T ReturnLength
);
typedef VOID(NTAPI *PKNORMAL_ROUTINE)(
	_In_opt_ PVOID NormalContext,
	_In_opt_ PVOID SystemArgument1,
	_In_opt_ PVOID SystemArgument2
);
typedef VOID(NTAPI *PKKERNEL_ROUTINE)(
	_In_ PKAPC *Apc,
	_Inout_ PKNORMAL_ROUTINE *NormalRoutine,
	_Inout_ PVOID *NormalContext,
	_Inout_ PVOID *SystemArgument1,
	_Inout_ PVOID *SystemArgument2
);
typedef VOID(NTAPI *PKRUNDOWN_ROUTINE)(_In_opt_ PKAPC Apc);
typedef NTSTATUS(NTAPI* PLdrLoadDll)(
	_In_opt_ PWSTR DllPath,
	_In_opt_ PULONG DllCharacteristics,
	_In_ PUNICODE_STRING DllName,
	_Out_ PHANDLE DllHandle
);
typedef BOOLEAN(NTAPI *PKeAlertThread)(
	_In_ PKTHREAD Thread,
	_In_ KPROCESSOR_MODE AlertMode
);
typedef VOID(NTAPI *PKeInitializeApc)(
	_Out_ PKAPC Apc,
	_In_ PKTHREAD Thread,
	_In_ KAPC_ENVIRONMENT Environment,
	_In_ PKKERNEL_ROUTINE KernelRoutine,
	_In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
	_In_opt_ PKNORMAL_ROUTINE NormalRoutine,
	_In_opt_ KPROCESSOR_MODE ProcessorMode,
	_In_opt_ PVOID NormalContext
);
typedef BOOLEAN(NTAPI *PKeInsertQueueApc)(
	_In_ PKAPC Apc,
	_In_opt_ PVOID SystemArgument1,
	_In_opt_ PVOID SystemArgument2,
	_In_ KPRIORITY Increment
);
typedef PVOID(NTAPI* PExAllocatePool2)(
	_In_ POOL_FLAGS Flags,
	_In_ SIZE_T NumberOfBytes,
	_In_ ULONG Tag
);
typedef PVOID(NTAPI* PExAllocatePoolWithTag)(
	_In_ __drv_strictTypeMatch(__drv_typeExpr)POOL_TYPE PoolType,
	_In_ SIZE_T NumberOfBytes,
	_In_ ULONG Tag
);

//
// API address storage
//
PLdrLoadDll LdrLoadDll = nullptr;
PZwQuerySection ZwQuerySection = nullptr;
PKeAlertThread KeAlertThread = nullptr;
PKeInitializeApc KeInitializeApc = nullptr;
PKeInsertQueueApc KeInsertQueueApc = nullptr;
PExAllocatePool2 pExAllocatePool2 = nullptr;
PExAllocatePoolWithTag pExAllocatePoolWithTag = nullptr;

//
// Prototypes
//
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS OnCreateClose(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp
);
NTSTATUS OnDeviceControl(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp
);
PVOID GetNtdllRoutineAddress(_In_ const PCHAR apiName);
VOID NTAPI ApcRoutine(
	_In_ PKAPC *Apc,
	_Inout_ PKNORMAL_ROUTINE* NormalRoutine,
	_Inout_ PVOID* NormalContext,
	_Inout_ PVOID* SystemArgument1,
	_Inout_ PVOID* SystemArgument2
);
PVOID AllocateNonPagedPool(_In_ SIZE_T nPoolSize);

//
// Driver routines
//
extern "C"
NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS ntstatus = STATUS_FAILED_DRIVER_ENTRY;
	PDEVICE_OBJECT pDeviceObject = nullptr;

#ifndef _WIN64
	KdPrint((DRIVER_PREFIX "32bit OS is not supported.\n"));
	return STATUS_NOT_SUPPORTED;
#endif

	do
	{
		UNICODE_STRING devicePath = RTL_CONSTANT_STRING(DEVICE_PATH);
		UNICODE_STRING symlinkPath = RTL_CONSTANT_STRING(SYMLINK_PATH);
		UNICODE_STRING routineName{ 0 };

		::RtlInitUnicodeString(&routineName, L"ZwQuerySection");
		ZwQuerySection = (PZwQuerySection)::MmGetSystemRoutineAddress(&routineName);

		if (ZwQuerySection == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve %wZ() API.\n", routineName));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "%wZ() API is at 0x%p.\n", routineName, (PVOID)ZwQuerySection));
		}

		LdrLoadDll = (PLdrLoadDll)GetNtdllRoutineAddress(const_cast<PCHAR>("LdrLoadDll"));

		if (LdrLoadDll == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve LdrLoadDll() API.\n"));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "LdrLoadDll() API should be at 0x%p.\n", (PVOID)LdrLoadDll));
		}

		::RtlInitUnicodeString(&routineName, L"KeAlertThread");
		KeAlertThread = (PKeAlertThread)::MmGetSystemRoutineAddress(&routineName);

		if (KeAlertThread == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve %wZ() API.\n", routineName));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "%wZ() API is at 0x%p.\n", routineName, (PVOID)KeAlertThread));
		}

		::RtlInitUnicodeString(&routineName, L"KeInitializeApc");
		KeInitializeApc = (PKeInitializeApc)::MmGetSystemRoutineAddress(&routineName);

		if (KeInitializeApc == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve %wZ() API.\n", routineName));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "%wZ() API is at 0x%p.\n", routineName, (PVOID)KeInitializeApc));
		}

		::RtlInitUnicodeString(&routineName, L"KeInsertQueueApc");
		KeInsertQueueApc = (PKeInsertQueueApc)::MmGetSystemRoutineAddress(&routineName);

		if (KeInsertQueueApc == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve %wZ() API.\n", routineName));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "%wZ() API is at 0x%p.\n", routineName, (PVOID)KeInsertQueueApc));
		}

		::RtlInitUnicodeString(&routineName, L"ExAllocatePool2");
		pExAllocatePool2 = (PExAllocatePool2)::MmGetSystemRoutineAddress(&routineName);

		if (pExAllocatePool2 == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve ExAllocatePool2() API. Trying to resolve ExAllocatePoolWithTag() API.\n"));
			::RtlInitUnicodeString(&routineName, L"ExAllocatePoolWithTag");
			pExAllocatePoolWithTag = (PExAllocatePoolWithTag)::MmGetSystemRoutineAddress(&routineName);
		}

		if (pExAllocatePool2)
		{
			KdPrint((DRIVER_PREFIX "ExAllocatePool2() API is at 0x%p.\n", (PVOID)pExAllocatePool2));
		}
		else if (pExAllocatePoolWithTag)
		{
			KdPrint((DRIVER_PREFIX "ExAllocatePoolWithTag() API is at 0x%p.\n", (PVOID)pExAllocatePoolWithTag));
		}
		else
		{
			KdPrint((DRIVER_PREFIX "Failed to resolve ExAllocatePool2() API and ExAllocatePoolWithTag() API.\n"));
			break;
		}

		ntstatus = ::IoCreateDevice(
			DriverObject,
			NULL,
			&devicePath,
			FILE_DEVICE_UNKNOWN,
			NULL,
			FALSE,
			&pDeviceObject);

		if (!NT_SUCCESS(ntstatus))
		{
			pDeviceObject = nullptr;
			KdPrint((DRIVER_PREFIX "Failed to create device (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}

		ntstatus = ::IoCreateSymbolicLink(&symlinkPath, &devicePath);

		if (!NT_SUCCESS(ntstatus))
		{
			KdPrint((DRIVER_PREFIX "Failed to create symbolic link (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}

		DriverObject->DriverUnload = DriverUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = OnCreateClose;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = OnCreateClose;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

		KdPrint((DRIVER_PREFIX "Driver is loaded successfully.\n"));
	} while (false);

	if (!NT_SUCCESS(ntstatus) && (pDeviceObject != nullptr))
		::IoDeleteDevice(pDeviceObject);

	return ntstatus;
}


void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symlinkPath = RTL_CONSTANT_STRING(SYMLINK_PATH);
	::IoDeleteSymbolicLink(&symlinkPath);
	::IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint((DRIVER_PREFIX "Driver is unloaded.\n"));
}


NTSTATUS OnCreateClose(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS ntstatus = STATUS_SUCCESS;
	Irp->IoStatus.Status = ntstatus;
	Irp->IoStatus.Information = 0u;
	IoCompleteRequest(Irp, 0);

	return ntstatus;
}


NTSTATUS OnDeviceControl(
	_Inout_ PDEVICE_OBJECT DeviceObject,
	_Inout_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS ntstatus = STATUS_INVALID_DEVICE_REQUEST;
	PIO_STACK_LOCATION irpSp = ::IoGetCurrentIrpStackLocation(Irp);
	auto& dic = irpSp->Parameters.DeviceIoControl;
	ULONG_PTR info = NULL;
	PETHREAD pThread = nullptr;
	PVOID pPathBuffer = nullptr;
	PKAPC pKapc = nullptr;
	HANDLE hProcess = nullptr;
	struct : UNICODE_STRING { WCHAR buf[256]; } packedUnicodeString{ };
	SIZE_T nBufferSize = sizeof(UNICODE_STRING) + (sizeof(WCHAR) * 256);
	KAPC_STATE apcState{ 0 };
	CLIENT_ID clientId{ 0 };
	OBJECT_ATTRIBUTES objectAttributes{ 0 };
	objectAttributes.Length = (ULONG)sizeof(OBJECT_ATTRIBUTES);
	objectAttributes.Attributes = OBJ_KERNEL_HANDLE;

	switch (dic.IoControlCode)
	{
	case IOCTL_INJECT_LIBRARY:
		if (dic.InputBufferLength < sizeof(INJECT_CONTEXT))
		{
			ntstatus = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		pKapc = (PKAPC)AllocateNonPagedPool(sizeof(KAPC));

		if (pKapc == nullptr)
		{
			KdPrint((DRIVER_PREFIX "Failed to allocate non-paged pool for _KAPC.\n"));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "Non-paged pool for _KAPC is allocated at 0x%p.\n", pKapc));
		}

		ntstatus = ::PsLookupThreadByThreadId(
			ULongToHandle(((PINJECT_CONTEXT)Irp->AssociatedIrp.SystemBuffer)->ThreadId),
			&pThread);

		if (!NT_SUCCESS(ntstatus))
		{
			pThread = nullptr;
			KdPrint((DRIVER_PREFIX "Failed to lookup nt!_ETHREAD for thread ID %u (NTSTATUS = 0x%08X).\n",
				((PINJECT_CONTEXT)Irp->AssociatedIrp.SystemBuffer)->ThreadId,
				ntstatus));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "nt!_ETHREAD for thread ID %u is at 0x%p.\n",
				((PINJECT_CONTEXT)Irp->AssociatedIrp.SystemBuffer)->ThreadId,
				pThread));
		}

		clientId.UniqueProcess = ::PsGetThreadProcessId(pThread);
		ntstatus = ::ZwOpenProcess(
			&hProcess,
			PROCESS_ALL_ACCESS,
			&objectAttributes,
			&clientId);

		if (!NT_SUCCESS(ntstatus))
		{
			hProcess = nullptr;
			KdPrint((DRIVER_PREFIX "Failed to lookup get process handle of thread (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "Got process handle for PID %u.\n", HandleToULong(clientId.UniqueProcess)));
		}

		ntstatus = ::ZwAllocateVirtualMemory(
			hProcess,
			&pPathBuffer,
			NULL,
			&nBufferSize,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE);

		if (!NT_SUCCESS(ntstatus))
		{
			pPathBuffer = nullptr;
			KdPrint((DRIVER_PREFIX "Failed to allocate buffer in PID %u (NTSTATUS = 0x%08X).\n",
				HandleToULong(clientId.UniqueProcess),
				ntstatus));
			break;
		}

		KdPrint((DRIVER_PREFIX "%Iu bytes buffer is allocate at 0x%p in PID %u.\n",
			nBufferSize,
			pPathBuffer,
			HandleToULong(clientId.UniqueProcess)));

		// Build packed _UNICODE_STRING data in kernel space before writing it in user space
		packedUnicodeString.Length = 0u;
		packedUnicodeString.MaximumLength = (USHORT)(sizeof(WCHAR) * 256);
		packedUnicodeString.Buffer = (PWCH)((ULONG_PTR)pPathBuffer + sizeof(UNICODE_STRING));
		::memcpy(&packedUnicodeString.buf, &((PINJECT_CONTEXT)Irp->AssociatedIrp.SystemBuffer)->LibraryPath, sizeof(WCHAR) * 256);

		for (auto idx = 0; idx < 256; idx++)
		{
			if (packedUnicodeString.buf[idx] == 0u)
				break;
			else
				packedUnicodeString.Length += sizeof(WCHAR);
		}

		// Copy packed _UNICODE_STRING data to user space
		::KeStackAttachProcess(::PsGetThreadProcess(pThread), &apcState);

		__try
		{
			::memcpy(pPathBuffer, &packedUnicodeString, sizeof(UNICODE_STRING) + (sizeof(WCHAR) * 256));
			KdPrint((DRIVER_PREFIX "Library to inject: %wZ.\n", (PUNICODE_STRING)pPathBuffer));
			ntstatus = STATUS_SUCCESS;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint((DRIVER_PREFIX "Access violation in user space.\n"));
			ntstatus = STATUS_ACCESS_VIOLATION;
		}

		::KeUnstackDetachProcess(&apcState);

		if (!NT_SUCCESS(ntstatus))
			break;

		KeInitializeApc(
			pKapc,
			pThread,
			OriginalApcEnvironment,
			ApcRoutine,
			nullptr,
			(PKNORMAL_ROUTINE)LdrLoadDll,
			UserMode,
			nullptr);

		if (KeInsertQueueApc(pKapc, nullptr, pPathBuffer, IO_NO_INCREMENT))
		{
			KdPrint((DRIVER_PREFIX "APC queue is inserted successfully.\n"));
			KeAlertThread(pThread, UserMode);
		}
		else
		{
			KdPrint((DRIVER_PREFIX "Failed to insert APC queue.\n"));
		}
	}

	if (!NT_SUCCESS(ntstatus) && (pKapc != nullptr))
		::ExFreePoolWithTag(pKapc, (ULONG)DRIVER_TAG);

	if (hProcess != nullptr)
	{
		if (!NT_SUCCESS(ntstatus) && (pPathBuffer != nullptr))
		{
			nBufferSize = NULL;
			::ZwFreeVirtualMemory(hProcess, &pPathBuffer, &nBufferSize, MEM_RELEASE);
		}

		::ZwClose(hProcess);
	}

	if (pThread != nullptr)
		ObDereferenceObject(pThread);

	Irp->IoStatus.Status = ntstatus;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);

	return ntstatus;
}


//
// Helper functions
//
PVOID AllocateNonPagedPool(_In_ SIZE_T nPoolSize)
{
	PVOID pNonPagedPool = nullptr;

	// ExAllocatePool2 API was introduced from Windows 10 2004.
	// Use ExAllocatePoolWithTag API on old OSes.
	if (pExAllocatePool2)
	{
		pNonPagedPool = pExAllocatePool2(POOL_FLAG_NON_PAGED, nPoolSize, (ULONG)DRIVER_TAG);
	}
	else if (pExAllocatePoolWithTag)
	{
		pNonPagedPool = pExAllocatePoolWithTag(NonPagedPool, nPoolSize, (ULONG)DRIVER_TAG);

		if (pNonPagedPool)
			RtlZeroMemory(pNonPagedPool, nPoolSize);
	}

	return pNonPagedPool;
}


PVOID GetNtdllRoutineAddress(_In_ const PCHAR apiName)
{
	HANDLE hSection = nullptr;
	HANDLE hSystem = nullptr;
	PVOID pRoutine = nullptr;

	do
	{
		NTSTATUS ntstatus;
		PVOID pNtdll = nullptr;
		PVOID pSectionBase = nullptr;
		PEPROCESS pSystem = nullptr;
		SIZE_T nViewSize = NULL;
		KAPC_STATE apcState{ 0 };
		SIZE_T nInfoLength = sizeof(SECTION_IMAGE_INFORMATION);
		SECTION_IMAGE_INFORMATION sectionImageInfo{ 0 };
		CLIENT_ID clientId { ULongToHandle(4u), nullptr };
		UNICODE_STRING objectPath = RTL_CONSTANT_STRING(L"\\KnownDlls\\ntdll.dll");
		OBJECT_ATTRIBUTES objectAttributes{ 0 };
		InitializeObjectAttributes(
			&objectAttributes,
			&objectPath,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			nullptr,
			nullptr);

		//
		// Get user space address for ntdll.dll from KnownDlls section
		//
		ntstatus = ::ZwOpenSection(&hSection, SECTION_QUERY | SECTION_MAP_READ, &objectAttributes);

		if (!NT_SUCCESS(ntstatus))
		{
			hSection = nullptr;
			KdPrint((DRIVER_PREFIX "Failed to ZwOpenSection() (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}

		ntstatus = ZwQuerySection(
			hSection,
			SectionImageInformation,
			&sectionImageInfo,
			nInfoLength,
			&nInfoLength);

		if (!NT_SUCCESS(ntstatus))
		{
			KdPrint((DRIVER_PREFIX "Failed to ZwQuerySection() (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}
		else
		{
			pNtdll = sectionImageInfo.TransferAddress;
			KdPrint((DRIVER_PREFIX "ntdll.dll should be at 0x%p.\n", pNtdll));
		}

		//
		// Get routine address by PE analyzing from mapped \KnownDlls\ntdll.dll in System process
		//
		::memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
		objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
		objectAttributes.Attributes = OBJ_KERNEL_HANDLE;

		ntstatus = ::ZwOpenProcess(&hSystem, PROCESS_ALL_ACCESS, &objectAttributes, &clientId);

		if (!NT_SUCCESS(ntstatus))
		{
			hSystem = nullptr;
			KdPrint((DRIVER_PREFIX "Failed to ZwOpenProcess() for System (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "Got System process handle 0x%p.\n", hSystem));
		}

		ntstatus = ::ZwMapViewOfSection(
			hSection,
			hSystem,
			&pSectionBase,
			0u,
			0u,
			nullptr,
			&nViewSize,
			ViewUnmap,
			NULL,
			PAGE_READWRITE);

		if (!NT_SUCCESS(ntstatus) && (ntstatus != STATUS_IMAGE_NOT_AT_BASE))
		{
			KdPrint((DRIVER_PREFIX "Failed to ZwMapViewOfSection() for System (NTSTATUS = 0x%08X).\n", ntstatus));
			break;
		}
		else
		{
			KdPrint((DRIVER_PREFIX "ntdll.dll section is mapped at 0x%p in System.\n", pSectionBase));
		}

		::PsLookupProcessByProcessId(ULongToHandle(4u), &pSystem);
		::KeStackAttachProcess(pSystem, &apcState);

		__try
		{
			if (*(USHORT*)pSectionBase == 0x5A4D)
			{
				auto e_lfanew = ((PIMAGE_DOS_HEADER)pSectionBase)->e_lfanew;
				auto pImageNtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)pSectionBase + e_lfanew);
				auto nExportDirectoryOffset = pImageNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress;
				auto pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)pSectionBase + nExportDirectoryOffset);
				auto pOrdinals = (USHORT*)((ULONG_PTR)pSectionBase + pExportDirectory->AddressOfNameOrdinals);
				auto pNames = (ULONG*)((ULONG_PTR)pSectionBase + pExportDirectory->AddressOfNames);
				auto pFunctions = (ULONG*)((ULONG_PTR)pSectionBase + pExportDirectory->AddressOfFunctions);
				auto nEntries = pExportDirectory->NumberOfNames;
				auto nStrLen = ::strlen(apiName);

				for (auto idx = 0u; idx < nEntries; idx++)
				{
					auto functionName = (PCHAR)((ULONG_PTR)pSectionBase + pNames[idx]);

					if (::strncmp(functionName, apiName, nStrLen) == 0)
					{
						pRoutine = (PVOID)((ULONG_PTR)pNtdll + pFunctions[pOrdinals[idx]]);
						break;
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint((DRIVER_PREFIX "Access violation in user space.\n"));
		}
		
		::KeUnstackDetachProcess(&apcState);
		ObDereferenceObject(pSystem);

		ntstatus = ::ZwUnmapViewOfSection(hSystem, pSectionBase);

		if (!NT_SUCCESS(ntstatus))
			KdPrint((DRIVER_PREFIX "Failed to ZwUnmapViewOfSection() for System (NTSTATUS = 0x%08X).\n", ntstatus));
		else
			KdPrint((DRIVER_PREFIX "ntdll.dll section is unmapped from System.\n"));
	} while (false);

	if (hSystem != nullptr)
		::ZwClose(hSystem);

	if (hSection != nullptr)
		::ZwClose(hSection);

	return pRoutine;
}


VOID NTAPI ApcRoutine(
	_In_ PKAPC* Apc,
	_Inout_ PKNORMAL_ROUTINE* NormalRoutine,
	_Inout_ PVOID* NormalContext,
	_Inout_ PVOID* SystemArgument1,
	_Inout_ PVOID* SystemArgument2)
{
	UNREFERENCED_PARAMETER(Apc);
	UNREFERENCED_PARAMETER(NormalRoutine);
	UNREFERENCED_PARAMETER(NormalContext);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	KdPrint((DRIVER_PREFIX "Kernel APC routine is called.\n"));
}

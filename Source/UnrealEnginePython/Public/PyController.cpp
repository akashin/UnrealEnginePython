#include "UnrealEnginePythonPrivatePCH.h"
#include "PyController.h"


APyController::APyController()
{
	PrimaryActorTick.bCanEverTick = true;

	PythonTickForceDisabled = false;
	PythonDisableAutoBinding = false;
	
}


// Called when the game starts
void APyController::BeginPlay()
{
	Super::BeginPlay();

	// ...

	if (PythonModule.IsEmpty())
		return;

	FScopePythonGIL gil;

	py_uobject = ue_get_python_wrapper(this);
	if (!py_uobject) {
		unreal_engine_py_log_error();
		return;
	}

	PyObject *py_controller_module = PyImport_ImportModule(TCHAR_TO_UTF8(*PythonModule));
	if (!py_controller_module) {
		unreal_engine_py_log_error();
		return;
	}

#if WITH_EDITOR
	// todo implement autoreload with a dictionary of module timestamps
	py_controller_module = PyImport_ReloadModule(py_controller_module);
	if (!py_controller_module) {
		unreal_engine_py_log_error();
		return;
	}
#endif

	if (PythonClass.IsEmpty())
		return;

	PyObject *py_controller_module_dict = PyModule_GetDict(py_controller_module);
	PyObject *py_controller_class = PyDict_GetItemString(py_controller_module_dict, TCHAR_TO_UTF8(*PythonClass));

	if (!py_controller_class) {
		UE_LOG(LogPython, Error, TEXT("Unable to find class %s in module %s"), *PythonClass, *PythonModule);
		return;
	}

	py_controller_instance = PyObject_CallObject(py_controller_class, NULL);
	if (!py_controller_instance) {
		unreal_engine_py_log_error();
		return;
	}

	py_uobject->py_proxy = py_controller_instance;

	PyObject_SetAttrString(py_controller_instance, (char *)"uobject", (PyObject *)py_uobject);


	// disable ticking if not required
	if (!PyObject_HasAttrString(py_controller_instance, (char *)"tick") || PythonTickForceDisabled) {
		SetActorTickEnabled(false);
	}

	if (!PythonDisableAutoBinding)
		ue_autobind_events_for_pyclass(py_uobject, py_controller_instance);

	if (!PyObject_HasAttrString(py_controller_instance, (char *)"begin_play"))
		return;

	PyObject *bp_ret = PyObject_CallMethod(py_controller_instance, (char *)"begin_play", NULL);
	if (!bp_ret) {
		unreal_engine_py_log_error();
		return;
	}
	Py_DECREF(bp_ret);
}


// Called every frame
void APyController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!py_controller_instance)
		return;

	FScopePythonGIL gil;

	PyObject *ret = PyObject_CallMethod(py_controller_instance, (char *)"tick", (char *)"f", DeltaTime);
	if (!ret) {
		unreal_engine_py_log_error();
		return;
	}
	Py_DECREF(ret);

}


void APyController::CallPythonControllerMethod(FString method_name)
{
	if (!py_controller_instance)
		return;

	FScopePythonGIL gil;

	PyObject *ret = PyObject_CallMethod(py_controller_instance, TCHAR_TO_UTF8(*method_name), NULL);
	if (!ret) {
		unreal_engine_py_log_error();
		return;
	}
	Py_DECREF(ret);
}

bool APyController::CallPythonControllerMethodBool(FString method_name)
{
	if (!py_controller_instance)
		return false;

	FScopePythonGIL gil;

	PyObject *ret = PyObject_CallMethod(py_controller_instance, TCHAR_TO_UTF8(*method_name), NULL);
	if (!ret) {
		unreal_engine_py_log_error();
		return false;
	}

	if (PyObject_IsTrue(ret)) {
		Py_DECREF(ret);
		return true;
	}

	Py_DECREF(ret);
	return false;
}

FString APyController::CallPythonControllerMethodString(FString method_name)
{
	if (!py_controller_instance)
		return FString();

	FScopePythonGIL gil;

	PyObject *ret = PyObject_CallMethod(py_controller_instance, TCHAR_TO_UTF8(*method_name), NULL);
	if (!ret) {
		unreal_engine_py_log_error();
		return FString();
	}

	PyObject *py_str = PyObject_Str(ret);
	if (!py_str) {
		Py_DECREF(ret);
		return FString();
	}

	char *str_ret = PyUnicode_AsUTF8(py_str);

	FString ret_fstring = FString(UTF8_TO_TCHAR(str_ret));

	Py_DECREF(py_str);

	return ret_fstring;
}


APyController::~APyController()
{
	FScopePythonGIL gil;

	ue_pydelegates_cleanup(py_uobject);

#if UEPY_MEMORY_DEBUG
	if (py_controller_instance && py_controller_instance->ob_refcnt != 1) {
		UE_LOG(LogPython, Error, TEXT("Inconsistent Python AController wrapper refcnt = %d"), py_controller_instance->ob_refcnt);
	}
#endif
	Py_XDECREF(py_controller_instance);
	

#if UEPY_MEMORY_DEBUG
	UE_LOG(LogPython, Warning, TEXT("Python AController (mapped to %p) wrapper XDECREF'ed"), py_uobject ? py_uobject->ue_object : nullptr);
#endif

	// this could trigger the distruction of the python/uobject mapper
	Py_XDECREF(py_uobject);
}

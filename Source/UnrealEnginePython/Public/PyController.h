#pragma once


#include "GameFramework/Controller.h"

#include "PyController.generated.h"



UCLASS(BlueprintType, Blueprintable)
class APyController : public AController
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	APyController();
	~APyController();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere , Category = "Python")
	FString PythonModule;

	UPROPERTY(EditAnywhere, Category = "Python")
	FString PythonClass;

	UPROPERTY(EditAnywhere, Category = "Python")
	bool PythonTickForceDisabled;

	UPROPERTY(EditAnywhere, Category = "Python")
	bool PythonDisableAutoBinding;

	UFUNCTION(BlueprintCallable, Category = "Python")
	void CallPythonControllerMethod(FString method_name);

	UFUNCTION(BlueprintCallable, Category = "Python")
	bool CallPythonControllerMethodBool(FString method_name);

	UFUNCTION(BlueprintCallable, Category = "Python")
	FString CallPythonControllerMethodString(FString method_name);

private:
	PyObject *py_controller_instance;
	// mapped uobject, required for debug and advanced reflection
	ue_PyUObject *py_uobject;
};

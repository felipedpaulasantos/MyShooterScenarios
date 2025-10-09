// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyDebugLibrary.generated.h"

/**
 *
 */
UCLASS()
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMyDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Imprime uma string na tela com o nome da classe e o timestamp atual.
	 * @param WorldContextObject Fornece o contexto do mundo e do objeto que est� chamando esta fun��o.
	 * @param InString A string a ser impressa.
	 * @param bPrintToScreen Se deve imprimir na tela.
	 * @param bPrintToLog Se deve imprimir no log de sa�da.
	 * @param TextColor A cor do texto na tela.
	 * @param Duration A em segundos.
	*/
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", DevelopmentOnly, Keywords = "log print", AdvancedDisplay = "bPrintToScreen,bPrintToLog,TextColor,Duration"), Category = "My Debug")
	static void PrintStringAdvanced(const UObject* WorldContextObject, const FString& InString = "Hello", bool bPrintToScreen = true, bool bPrintToLog = true, FLinearColor TextColor = FLinearColor(0.0, 0.66, 1.0), float Duration = 2.f);
};

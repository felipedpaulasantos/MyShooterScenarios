// Fill out your copyright notice in the Description page of Project Settings.

#include "MyDebugLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"

void UMyDebugLibrary::PrintStringAdvanced(const UObject* WorldContextObject, const FString& InString,
                                          bool bPrintToScreen, bool bPrintToLog, FLinearColor TextColor, float Duration)
{
	// Se o WorldContextObject for nulo, usamos "NoContext" como fallback.
	FString ClassName = WorldContextObject ? WorldContextObject->GetClass()->GetName() : TEXT("NoContext");

	// Remover o prefixo padr�o "BP_" ou "C_" para deixar o nome mais limpo, se desejado.
	ClassName.RemoveFromStart(TEXT("BP_"));
	ClassName.RemoveFromStart(TEXT("C_"));

	// Obter o timestamp atual.
	FDateTime CurrentTime = FDateTime::Now();
	FString Timestamp = FString::Printf(TEXT("%02d:%02d:%02d"), CurrentTime.GetHour(), CurrentTime.GetMinute(),
	                                    CurrentTime.GetSecond());

	// Montar a string final no formato desejado.
	const FString FinalString = FString::Printf(TEXT("[%s] [%s] %s"), *ClassName, *Timestamp, *InString);
	
	UKismetSystemLibrary::PrintString(WorldContextObject, FinalString, bPrintToScreen, bPrintToLog, TextColor,
	                                  Duration);
}

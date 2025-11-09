```cpp
// Exemplo: GA_Weapon_Fire_Pistol
FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, 1.0f, Context);
SpecHandle.Data->SetSetByCallerMagnitude(LyraGameplayTags::SetByCaller_Damage, 25.0f); // <-- AQUI!
ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
```

### 2. WeaponInstance/DataAsset
```cpp
// B_WeaponInstance_Pistol ou DA_Weapon_Pistol
UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
float WeaponBaseDamage = 25.0f; // <-- AQUI!
```

### 3. No código C++ da arma
```cpp
// ULyraWeaponInstance ou similar
UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
float BaseDamage = 25.0f; // <-- AQUI!
```

## Solução Recomendada

**NÃO tente** ler o dano do GameplayEffect se ele usa SetByCaller.

**Ao invés disso:**
1. Armazene o dano como propriedade da arma/ability
2. Exponha essa propriedade para Blueprint
3. Use o mesmo valor ao chamar `SetSetByCallerMagnitude()`

### Exemplo Blueprint:

**B_WeaponInstance_Pistol:**
```
Variável: BaseDamage (float) = 25.0
Exposição: BlueprintReadOnly
```

**GA_Weapon_Fire_Pistol:**
```
Get WeaponInstance
  -> Get BaseDamage
    -> Set By Caller Magnitude (Tag: SetByCaller.Damage, Magnitude: BaseDamage)
```

**Widget UI (mostra dano):**
```
Get WeaponInstance
  -> Get BaseDamage
    -> Format Text: "Damage: {0}"
```

## Compilação

```batch
# No terminal PowerShell/CMD:
cd F:\UnrealProjects\MyShooterScenarios
compile-ai-classes.bat
```

Ou use **Live Coding** no Editor (Ctrl + Alt + F11).

---
**Criado:** 2025-11-09  
**Projeto:** MyShooterScenarios (Lyra 5.6)
# GetBaseDamageFromDamageEffect - Referência Rápida

## Status Atual ✅
- **NÃO CAUSA CRASH** - Problema resolvido
- Detecta corretamente quando GameplayEffect usa **SetByCaller**
- Pode extrair valores de **ScalableFloat** quando disponível

## Como Usar em Blueprint

```cpp
// No seu Blueprint
UGameplayEffect* DamageEffect = GE_Damage_Pistol;
float BaseDamage = 0.0f;

bool Success = GetBaseDamageFromDamageEffect(DamageEffect, BaseDamage);

if (Success)
{
    // BaseDamage contém o valor extraído
    Print("Dano base: " + BaseDamage);
}
else
{
    // Não foi possível extrair - cheque os logs para saber o motivo
    // Provavelmente usa SetByCaller ou outra magnitude dinâmica
}
```

## Logs que Você Verá

### Para GE_Damage_Pistol (usa SetByCaller):
```
LogLyraGEBlueprintLib: Attempting to extract BaseDamage from GameplayEffect GE_Damage_Pistol_C
LogLyraGEBlueprintLib:   Found LyraDamageExecution
LogLyraGEBlueprintLib:   Found BaseDamage in CalculationModifiers
LogLyraGEBlueprintLib:   MagnitudeCalculationType = 3
Warning:   BaseDamage uses SetByCaller.
Warning:   The actual damage value is set at runtime via SetSetByCallerMagnitude().
Warning:   Check the weapon/ability code (e.g., GA_Weapon_Fire) to see the damage value being set.

Retorna: false
```

### Para GE com ScalableFloat:
```
LogLyraGEBlueprintLib: Attempting to extract BaseDamage from GameplayEffect GE_MyCustomDamage_C
LogLyraGEBlueprintLib:   Found LyraDamageExecution
LogLyraGEBlueprintLib:   Found BaseDamage in CalculationModifiers
LogLyraGEBlueprintLib:   MagnitudeCalculationType = 0
LogLyraGEBlueprintLib:   Successfully extracted BaseDamage from ScalableFloat: 25.0

Retorna: true, BaseDamage = 25.0
```

## Tipos de Magnitude Detectados

| Tipo | Enum Value | Pode Extrair? | Observações |
|------|------------|---------------|-------------|
| **ScalableFloat** | 0 | ✅ SIM | Valor fixo ou via DataTable |
| **AttributeBased** | 1 | ❌ NÃO | Depende de atributo runtime |
| **CustomCalculationClass** | 2 | ❌ NÃO | Usa classe custom de cálculo |
| **SetByCaller** | 3 | ❌ NÃO | Definido em runtime pela Ability |

## SetByCaller - Onde Está o Valor Real?

Se o GE usa **SetByCaller**, o valor de dano NÃO está no GameplayEffect. Procure em:

### 1. Ability que aplica o efeito


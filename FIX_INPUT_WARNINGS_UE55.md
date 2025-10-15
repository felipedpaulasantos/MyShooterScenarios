# Como Corrigir os Avisos de Input no UE 5.5

## ⚠️ Problema Identificado

Os avisos de build mostram que o jogo está tentando carregar assets do tipo **PlayerMappableInputConfig** (PMI) que foram removidos no UE 5.5:

```
Warning: Failed to load asset [/Game/Core/Input/Configs/PMI_Default_KBM.PMI_Default_KBM]
Warning: Failed to load asset [/Game/Core/Input/Configs/PMI_Default_Gamepad.PMI_Default_Gamepad]
Warning: Failed to load asset [/ShooterCore/Input/Configs/PMI_ShooterDefaultConfig_KBM.PMI_ShooterDefaultConfig_KBM]
Warning: Failed to load asset [/ShooterCore/Input/Configs/PMI_ShooterDefaultConfig_Gamepad.PMI_ShooterDefaultConfig_Gamepad]
```

## ✅ O Que Foi Corrigido no Código C++

1. **LyraHeroComponent.cpp**: Removido o loop que tentava carregar `DefaultInputConfigs` (array de PMI configs antigos)
2. **Sistema simplificado**: Agora usa apenas o `InputConfig` do `PawnData` + `GameFeatureAction_AddInputContextMapping`

## 🔧 Como Corrigir os Blueprints (NO EDITOR UNREAL)

### Passo 1: Limpar o Array DefaultInputConfigs

Os seguintes Blueprints podem ter referências aos PMI configs antigos na propriedade `DefaultInputConfigs`:

1. **Abra o Editor Unreal**
2. **Procure e abra estes Blueprints:**
   - `B_Hero_ShooterMannequin` (ou qualquer Blueprint que usa ULyraHeroComponent)
   - Blueprints de personagens em `/Game/Characters/`
   - Experience Definitions que configuram inputs

3. **Para cada Blueprint:**
   - Selecione o componente `LyraHeroComponent`
   - No painel Details, procure a propriedade `Default Input Configs`
   - **Limpe completamente este array** (remova todos os elementos)
   - **Salve o Blueprint**

### Passo 2: Verificar Game Feature Actions

Os Input Mapping Contexts devem estar configurados através de **Game Feature Actions**, não mais através de PMI configs.

1. **Abra seus Game Feature Data assets** (ex: `MyShooterFeaturePlugin.uasset`)
2. **Verifique se existe um `GameFeatureAction_AddInputContextMapping`**
3. **Se não existir, adicione um:**
   - Adicione uma nova Action do tipo `Add Input Mapping`
   - Configure os Input Mappings:
     - Para Keyboard/Mouse: `/Game/Core/Input/Contexts/IMC_Default_KBM` (ou similar)
     - Para Gamepad: `/Game/Core/Input/Contexts/IMC_Default_Gamepad` (ou similar)
     - Priority: 0

### Passo 3: Verificar Se os Input Mapping Contexts Existem

Os Input Mapping Contexts (IMC_*) devem existir e conter os mapeamentos de teclas:

1. **Navegue para** `/Game/Core/Input/` ou `/ShooterCore/Input/`
2. **Procure por assets do tipo `InputMappingContext`** (IMC_*)
3. **Se não existirem, você precisa criar:**
   - Clique direito → Input → Input Mapping Context
   - Nomeie como `IMC_Default_KBM` e `IMC_Default_Gamepad`
   - Adicione os mapeamentos de teclas dentro deles

## 📋 Como Funciona o Sistema de Input no UE 5.5

### Estrutura Antiga (UE 5.4) - ❌ NÃO USAR MAIS
```
PlayerMappableInputConfig (PMI_*) 
  └─> Registrado via DefaultInputConfigs array
```

### Estrutura Nova (UE 5.5) - ✅ USAR
```
PawnData
  └─> InputConfig (LyraInputConfig)
        └─> Contém Input Actions (botões/ações individuais)

Game Feature Action
  └─> GameFeatureAction_AddInputContextMapping
        └─> InputMappingContext (IMC_*)
              └─> Mapeia Input Actions para teclas físicas
```

## 🎯 Fluxo de Como os Inputs São Carregados

1. **PawnData** define quais **Input Actions** o personagem usa (ex: Move, Jump, Fire)
2. **GameFeatureAction_AddInputContextMapping** adiciona os **Input Mapping Contexts** que mapeiam essas actions para teclas (ex: W = Forward, Space = Jump)
3. **LyraInputComponent::AddInputMappings** (que foi corrigido) registra os custom key bindings do jogador
4. **LyraHeroComponent::InitializePlayerInput** faz o binding final de tudo

## ✅ Verificação Final

Após fazer as mudanças:

1. **Compile o código** (já foi corrigido)
2. **Abra o Editor Unreal**
3. **Limpe os arrays DefaultInputConfigs** nos Blueprints
4. **Configure os GameFeatureActions** com os IMC corretos
5. **PIE (Play In Editor)** e teste os inputs
6. **Verifique o log** - não deve mais mostrar os avisos de PMI

## 📝 Notas Importantes

- Os avisos são **NÃO-FATAIS** - o jogo pode funcionar mesmo com eles, mas é melhor corrigir
- O sistema antigo de PMI foi **completamente removido** pela Epic no UE 5.5
- Todos os projetos Lyra/ShooterCore precisam migrar para o novo sistema
- Se você não tem os IMC_* assets, pode ser que eles tenham sido deletados - você precisará recriá-los

## 🆘 Se os Inputs Ainda Não Funcionarem

Se após todas as correções os inputs ainda não funcionarem:

1. **Verifique o Output Log** por erros relacionados a Enhanced Input
2. **Confirme que o Enhanced Input Plugin está habilitado** (Project Settings → Plugins)
3. **Verifique se o PawnData tem um InputConfig válido** configurado
4. **Verifique se existe pelo menos um GameFeatureAction_AddInputContextMapping** ativo

---

**Data da Correção**: 2025-10-14  
**Versão do Engine**: Unreal Engine 5.5  
**Arquivos C++ Corrigidos**:
- `LyraHeroComponent.cpp` - Removido código deprecated do PMI system
- `LyraAssetManager.h/.cpp` - Corrigido exception 0x80000003
- `LyraGameSettingRegistry_MouseAndKeyboard.cpp` - Adicionados includes para UE 5.5
- `LyraSettingKeyboardInput.h` - Adicionado include PlayerMappableKeySettings.h


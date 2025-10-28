# CHANGELOG - Sistema de Look Around

## [v1.1] - 2025-10-27 - ROTAÇÃO SUAVE

### ✅ Corrigido
- **Rotação instantânea/muito rápida** → Agora rotação suave e controlada
- Interpolação linear simples → `RInterpConstantTo` (velocidade constante)
- Velocidade padrão muito alta (90°/s) → Reduzida para 45°/s

### ➕ Adicionado
- Nova propriedade: `bUseConstantRotationSpeed` (true por padrão)
  - TRUE: Rotação a velocidade constante (previsível)
  - FALSE: Rotação com suavização exponencial (cinematográfico)
- Detecção melhorada de chegada ao alvo (< 1 grau = completo)
- Atualização contínua da rotação inicial para interpolação mais suave

### 🔧 Modificado
- **BTService_LookAround.h:**
  - Adicionada propriedade `bool bUseConstantRotationSpeed`
  
- **BTService_LookAround.cpp:**
  - Construtor: `RotationSpeed` padrão 90→45, `bUseConstantRotationSpeed = true`
  - `TickNode()`: Nova lógica de interpolação com dois modos
  - Usa `FMath::RInterpConstantTo` para modo constante
  - Usa `FMath::RInterpTo` para modo suave
  - Usa `FMath::FindDeltaAngleDegrees` para detecção de chegada

### 📖 Documentação
- Criado: `CORRECAO_ROTACAO_SUAVE.md` - Detalhes técnicos completos
- Criado: `QUICK_REF_ROTACAO_SUAVE.md` - Referência rápida
- Atualizado: `IMPLEMENTACAO_BOT_LOOK_AROUND.md` - Novas propriedades e valores

---

## [v1.0] - 2025-10-27 - LANÇAMENTO INICIAL

### ✅ Implementado
- **BTService_LookAround** - Service para rotação contínua da visão
- **BTTask_SetFocusToRandomDirection** - Task para definir ponto focal aleatório
- Sistema de memória de nó com `FLookAroundMemory`
- Propriedades configuráveis via Blueprint
- Suporte para Blackboard Focus Target Key
- Opção de rotacionar apenas visão ou corpo inteiro

### 🐛 Corrigido (pré-lançamento)
- Crash do editor por falta de `GetInstanceMemorySize()`
- Crash por falta de `InitializeMemory()`
- Uso incorreto de `INIT_SERVICE_NODE_NOTIFY_FLAGS()`
- Cast C-style → `reinterpret_cast`

### 📖 Documentação
- Criado: `README_BOT_LOOK_AROUND.md` - Resumo geral
- Criado: `IMPLEMENTACAO_BOT_LOOK_AROUND.md` - Guia completo
- Criado: `CORRECAO_CRASH_BTSERVICE.md` - Detalhes do crash corrigido
- Criado: `ESTRUTURA_BEHAVIOR_TREE_LOOK_AROUND.md` - Diagramas visuais
- Criado: `compile-ai-classes.bat` - Script de compilação

---

## Compatibilidade

- **Unreal Engine:** 5.6
- **Projeto:** Lyra-based (MY_SHOOTER)
- **Módulos requeridos:** AIModule (já incluído)

---

## Próximas Melhorias Planejadas

- [ ] Curvas de animação personalizáveis
- [ ] Velocidade diferente para Yaw vs Pitch
- [ ] Randomização da velocidade de rotação
- [ ] Aceleração inicial configurável
- [ ] Integração com AI Perception (olhar para estímulos)
- [ ] Sistema de peso por direção (olhar mais para frente)
- [ ] Memória de locais suspeitos

---

## Migração

### De v1.0 para v1.1

1. Recompile o projeto
2. Abra Behavior Trees existentes
3. A nova propriedade `Use Constant Rotation Speed` aparecerá (padrão: TRUE)
4. Ajuste `Rotation Speed` se necessário (novo padrão: 45°/s)
5. Teste e ajuste conforme necessário

**Sem breaking changes!** Todas as configurações antigas continuam funcionando.

---

## Créditos

Desenvolvido para projeto MY_SHOOTER (Lyra)  
Data: Outubro 2025  
Unreal Engine 5.6


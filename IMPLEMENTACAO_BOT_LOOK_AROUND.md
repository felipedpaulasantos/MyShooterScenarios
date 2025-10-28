﻿﻿# Sistema de "Olhar em Volta" para Bots - Unreal Engine 5.6 Lyra

## ⚠️ CORREÇÃO DE CRASH APLICADA
**Versão atualizada:** Os arquivos foram corrigidos para evitar crash relacionado à inicialização de memória do BTService. O problema foi resolvido implementando corretamente `GetInstanceMemorySize()` e `InitializeMemory()`.

## ⚠️ CORREÇÃO DE CRASH APLICADA
**Versão atualizada:** Os arquivos foram corrigidos para evitar crash relacionado à inicialização de memória do BTService. O problema foi resolvido implementando corretamente `GetInstanceMemorySize()` e `InitializeMemory()`.

## ⚠️ CORREÇÃO DE CRASH APLICADA
**Versão atualizada:** Os arquivos foram corrigidos para evitar crash relacionado à inicialização de memória do BTService. O problema foi resolvido implementando corretamente `GetInstanceMemorySize()` e `InitializeMemory()`.

## Objetivo
Fazer os bots (BP_MyCharacterWithAbilities) olharem em volta enquanto patrulham ou buscam pelo jogador, tornando-os mais realistas e alertas.

## Arquivos Criados

### 1. BTService_LookAround
**Localização:** `Source/LyraGame/AI/BTService_LookAround.h/cpp`

**Descrição:** Serviço de Behavior Tree que faz o bot olhar em direções aleatórias continuamente enquanto o nó está ativo.

**Propriedades Configuráveis:**
- **Min Time Between Looks** (2.0s padrão): Tempo mínimo entre mudanças de direção
- **Max Time Between Looks** (5.0s padrão): Tempo máximo entre mudanças de direção
- **Max Yaw Angle** (90° padrão): Ângulo máximo de rotação horizontal
- **Max Pitch Angle** (30° padrão): Ângulo máximo de rotação vertical
- **Rotation Speed** (45°/s padrão): Velocidade da rotação da visão em graus por segundo
- **Use Constant Rotation Speed** (true padrão): Se true, velocidade constante; se false, suavização exponencial
- **Rotate Controller Only** (false padrão): Se true, apenas rotaciona a visão; se false, rotaciona o corpo quando parado
- **Focus Target Key** (opcional): Blackboard key para verificar se há um alvo prioritário

### 2. BTTask_SetFocusToRandomDirection
**Localização:** `Source/LyraGame/AI/BTTask_SetFocusToRandomDirection.h/cpp`

**Descrição:** Task de Behavior Tree que define um ponto focal em uma direção aleatória próxima.

**Propriedades Configuráveis:**
- **Focus Distance** (500 unidades padrão): Distância do ponto focal
- **Max Yaw Deviation** (90° padrão): Desvio horizontal máximo
- **Max Pitch Deviation** (30° padrão): Desvio vertical máximo
- **Clear Previous Focus** (true padrão): Se limpa o foco anterior

## Como Implementar no Behavior Tree

### Opção 1: Usando BTService_LookAround (Recomendado)

1. **Abra a Behavior Tree** `BT_Bot_Midrange` no editor do Unreal
2. **Selecione o nó de Patrulha/Busca** (geralmente um Composite node como Sequence ou Selector)
3. **Adicione o Service:**
   - Clique com o botão direito no nó
   - Vá em "Add Service" → "Look Around While Patrolling"
4. **Configure as propriedades:**
   ```
   Min Time Between Looks: 2.0
   Max Time Between Looks: 4.0
   Max Yaw Angle: 75.0
   Max Pitch Angle: 25.0
   Rotation Speed: 90.0
   Rotate Controller Only: false
   Focus Target Key: TargetActor (se você tiver um alvo na blackboard)
   ```

**Estrutura Exemplo:**
```
Root
└── Selector
    ├── Sequence [Service: Look Around While Patrolling]
    │   ├── HasTarget? (Decorator)
    │   └── AttackTarget (Task)
    └── Sequence [Service: Look Around While Patrolling]
        ├── FindPatrolPoint (Task)
        ├── MoveTo PatrolPoint (Task)
        └── Wait (Task) - 2-5 segundos
```

### Opção 2: Usando BTTask_SetFocusToRandomDirection

1. **Abra a Behavior Tree** `BT_Bot_Midrange`
2. **Na sequência de patrulha**, adicione a task entre os waypoints:
   ```
   Sequence (Patrol)
   ├── FindPatrolPoint
   ├── Set Focus to Random Direction
   ├── MoveTo PatrolPoint
   ├── Set Focus to Random Direction
   └── Wait 3 segundos
   ```

### Opção 3: Combinação (Máximo Realismo)

Combine ambos para criar um comportamento mais natural:

```
Root
└── Selector
    ├── Sequence (Combat) [Service: Look Around]
    │   ├── HasTarget? (Decorator)
    │   ├── Set Focus to Target
    │   └── AttackTarget
    └── Sequence (Patrol) [Service: Look Around]
        ├── Set Focus to Random Direction
        ├── FindPatrolPoint
        ├── MoveTo PatrolPoint
        ├── Set Focus to Random Direction
        ├── Wait 2-4 segundos
        └── Loop
```

## Comportamentos Esperados

### Durante Patrulha
- O bot moverá a cabeça/corpo periodicamente olhando em diferentes direções
- Se estiver parado, rotaciona o corpo inteiro
- Se estiver se movendo, apenas rotaciona a visão (controller)

### Durante Busca
- Continua olhando em volta enquanto procura o jogador
- Quando encontrar o alvo, para de olhar aleatoriamente e foca no jogador

### Durante Combate
- Se configurado com Focus Target Key, para de olhar em volta quando há um alvo
- Retorna ao comportamento de olhar em volta quando perde o alvo

## Configurações Recomendadas por Tipo de Bot

### Bot Alerta (Guarda)
```
Rotation Speed: 60.0
Use Constant Rotation Speed: true
Max Time Between Looks: 3.0
Max Yaw Angle: 120.0
### Bot Patrulha Normal (RECOMENDADO)
Rotation Speed: 120.0
```

### Bot Patrulha Normal
```
Rotation Speed: 45.0
Use Constant Rotation Speed: true
Max Time Between Looks: 5.0
Max Yaw Angle: 90.0
Max Pitch Angle: 25.0
Rotation Speed: 90.0
```

### Bot Relaxado
```
Rotation Speed: 30.0
Use Constant Rotation Speed: false
```

### Bot Ágil/Scout
```
Min Time Between Looks: 1.0
Max Time Between Looks: 2.5
Max Yaw Angle: 120.0
Max Pitch Angle: 35.0
Rotation Speed: 90.0
Use Constant Rotation Speed: true
Max Time Between Looks: 7.0
Max Yaw Angle: 60.0
Max Pitch Angle: 20.0
Rotation Speed: 60.0
```

## Compilação

Para compilar as novas classes:

### O bot rotaciona muito rápido/instantaneamente
- **SOLUÇÃO:** Reduza o `Rotation Speed` (padrão: 45°/s)
- Experimente valores: 20-30 para lento, 45-60 para normal, 90+ para rápido
- Confirme que `Use Constant Rotation Speed` está marcado (TRUE)
- **Veja:** CORRECAO_ROTACAO_SUAVE.md para detalhes

### O bot rotaciona muito devagar
- Aumente o `Rotation Speed` (60-90°/s)
- Reduza `Min/Max Time Between Looks` para trocas mais frequentes

### A rotação parece robótica
- Desmarque `Use Constant Rotation Speed` (FALSE) para suavização exponencial
- Ajuste `Rotation Speed` para um valor menor (metade do atual)
- Combine com variação nos tempos entre looks
4. **Abra** `MY_SHOOTER.sln` no Visual Studio
5. **Compile** o projeto (Build → Build Solution)
6. **Abra o projeto** no Unreal Engine

Ou via linha de comando:
```cmd
cd F:\UnrealProjects\MyShooterScenarios
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" LyraGameEditor Win64 Development "F:\UnrealProjects\MyShooterScenarios\MY_SHOOTER.uproject" -waitmutex
```

## Troubleshooting

### O bot não está olhando em volta
- Verifique se o Service está anexado ao nó correto
- Confirme se a Behavior Tree está sendo executada
- Verifique se há um Target na blackboard que está bloqueando o look around

### O bot rotaciona muito rápido/lento
- Ajuste o `Rotation Speed`
- Ajuste `Min/Max Time Between Looks`

### O bot rotaciona o corpo enquanto se move
- Configure `Rotate Controller Only` como `true`
- Ou ajuste as configurações de orientação do Character Movement Component

## Melhorias Futuras (Opcional)

1. **Adicionar peso por direção:** Fazer o bot olhar mais para frente do que para trás
2. **Integração com Perception:** Olhar na direção de sons/estímulos
3. **Animações de cabeça:** Usar Control Rig para mover apenas a cabeça
4. **Estado de alerta:** Olhar mais rapidamente quando suspeito
5. **Memória de locais:** Olhar novamente para locais onde viu o jogador pela última vez

## Referências

- [Unreal Engine AI Documentation](https://docs.unrealengine.com/en-US/AI/)
- [Behavior Tree Quick Start](https://docs.unrealengine.com/en-US/behavior-tree-quick-start-guide/)
- [AI Perception System](https://docs.unrealengine.com/en-US/ai-perception-in-unreal-engine/)


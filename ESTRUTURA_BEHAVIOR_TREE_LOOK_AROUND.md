# Estrutura Recomendada - Behavior Tree com Look Around

## Diagrama da Behavior Tree

```
┌─────────────────────────────────────────────────────────────┐
│                         ROOT                                 │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    SELECTOR (Priority)                       │
│  "Decide entre Combate, Busca ou Patrulha"                  │
└─┬───────────────────────┬───────────────────────────────┬───┘
  │                       │                               │
  │                       │                               │
  ▼                       ▼                               ▼
┌──────────────┐    ┌──────────────┐             ┌──────────────┐
│   COMBATE    │    │    BUSCA     │             │  PATRULHA    │
│ (Prioridade  │    │ (Prioridade  │             │ (Prioridade  │
│   Alta)      │    │   Média)     │             │   Baixa)     │
└──────────────┘    └──────────────┘             └──────────────┘
```

---

## 1. Sequência de COMBATE (Prioridade Alta)

```
┌─────────────────────────────────────────────────────────────┐
│              SEQUENCE "Combat"                               │
│  🔧 SERVICE: Look Around (quando perde o alvo)              │
│  📌 Focus Target Key: "TargetActor"                         │
└─┬───────────────────────┬─────────────────────────────┬─────┘
  │                       │                             │
  ▼                       ▼                             ▼
┌────────────┐      ┌─────────────┐            ┌──────────────┐
│ DECORATOR  │      │   TASK      │            │    TASK      │
│ HasTarget? │ ───> │ SetFocusTo  │ ────────> │ AttackTarget │
│            │      │   Target    │            │              │
└────────────┘      └─────────────┘            └──────────────┘
```

**Configuração do Service:**
- Min Time: 1.0s (rápido, está em combate)
- Max Time: 2.0s
- Yaw: 120° (campo de visão amplo)
- Speed: 150°/s (rotação rápida)
- Focus Target Key: "TargetActor"

---

## 2. Sequência de BUSCA (Prioridade Média)

```
┌─────────────────────────────────────────────────────────────┐
│              SEQUENCE "Search for Enemy"                     │
│  🔧 SERVICE: Look Around (ativo durante busca)              │
└─┬─────────────────┬──────────────────┬─────────────────┬────┘
  │                 │                  │                 │
  ▼                 ▼                  ▼                 ▼
┌────────────┐  ┌─────────┐  ┌──────────────┐  ┌─────────────┐
│ DECORATOR  │  │  TASK   │  │    TASK      │  │    TASK     │
│HeardNoise? │─>│ LookAt  │─>│ MoveToLast   │─>│ Investigate │
│            │  │LastNoise│  │ KnownLocation│  │   (Wait)    │
└────────────┘  └─────────┘  └──────────────┘  └─────────────┘
```

**Configuração do Service:**
- Min Time: 1.5s
- Max Time: 3.0s
- Yaw: 120° (alerta, procurando)
- Pitch: 30°
- Speed: 120°/s
- Focus Target Key: None (sempre olha)

---

## 3. Sequência de PATRULHA (Prioridade Baixa)

```
┌─────────────────────────────────────────────────────────────────┐
│              SEQUENCE "Patrol"                                   │
│  🔧 SERVICE: Look Around (ativo durante patrulha)               │
└─┬────────────────┬────────────────┬────────────────┬───────────┘
  │                │                │                │
  ▼                ▼                ▼                ▼
┌─────────┐  ┌──────────┐  ┌─────────────┐  ┌────────────────┐
│  TASK   │  │   TASK   │  │    TASK     │  │     TASK       │
│ GetNext │─>│SetFocus  │─>│ MoveToNext  │─>│ WaitAtPoint    │
│Waypoint │  │ Random   │  │  Waypoint   │  │  (2-4 sec)     │
└─────────┘  └──────────┘  └─────────────┘  └────────────────┘
           SetFocusToRandomDirection        └──> ┌──────────┐
                                                  │SetFocus  │
                                                  │ Random   │
                                                  └──────────┘
                                            SetFocusToRandomDirection
```

**Configuração do Service:**
- Min Time: 2.0s (mais relaxado)
- Max Time: 5.0s
- Yaw: 90°
- Pitch: 25°
- Speed: 90°/s
- Focus Target Key: None

**Task SetFocusToRandomDirection (opcional):**
- Focus Distance: 500
- Max Yaw Deviation: 90°
- Max Pitch Deviation: 30°
- Clear Previous Focus: true

---

## Exemplo Completo em Pseudo-Blueprint

### Root → Selector

```
ROOT
└── Selector "Main AI Logic"
    ├── Sequence "Combat" ⭐ Prioridade 1
    │   ├── [Decorator] Blackboard Based Condition
    │   │   └── Key Query: TargetActor IS SET
    │   ├── [Service] Look Around While Patrolling
    │   │   ├── Min Time: 1.0
    │   │   ├── Max Time: 2.0
    │   │   ├── Yaw: 120°
    │   │   ├── Speed: 150°/s
    │   │   └── Focus Target Key: TargetActor
    │   ├── [Task] Set Focus to Target
    │   │   └── Blackboard Key: TargetActor
    │   └── [Task] Attack Target
    │       └── (Usa Gameplay Abilities)
    │
    ├── Sequence "Search" ⭐ Prioridade 2
    │   ├── [Decorator] Blackboard Based Condition
    │   │   └── Key Query: LastKnownLocation IS SET
    │   ├── [Service] Look Around While Patrolling
    │   │   ├── Min Time: 1.5
    │   │   ├── Max Time: 3.0
    │   │   ├── Yaw: 120°
    │   │   ├── Pitch: 30°
    │   │   └── Speed: 120°/s
    │   ├── [Task] Set Focus to Random Direction
    │   ├── [Task] Move To (LastKnownLocation)
    │   ├── [Task] Set Focus to Random Direction
    │   └── [Task] Wait (3-5 seconds)
    │
    └── Sequence "Patrol" ⭐ Prioridade 3 (Default)
        ├── [Service] Look Around While Patrolling
        │   ├── Min Time: 2.0
        │   ├── Max Time: 5.0
        │   ├── Yaw: 90°
        │   ├── Pitch: 25°
        │   └── Speed: 90°/s
        ├── [Task] Find Random Location
        │   └── Radius: 1000
        ├── [Task] Set Focus to Random Direction
        ├── [Task] Move To (Patrol Point)
        ├── [Task] Set Focus to Random Direction
        └── [Task] Wait (2-4 seconds)
```

---

## Blackboard Keys Necessárias

```
┌──────────────────────────────────────────────┐
│         Blackboard: BB_BotData               │
├──────────────────────────────────────────────┤
│ 🎯 TargetActor         (Object - Actor)      │
│ 📍 LastKnownLocation   (Vector)              │
│ 📍 PatrolPoint         (Vector)              │
│ 🔊 LastNoiseLocation   (Vector)              │
│ ⏱️  IsInCombat         (Bool)                │
│ ⏱️  IsSearching        (Bool)                │
└──────────────────────────────────────────────┘
```

---

## Comportamento Visual Esperado

### 🚶 Durante Patrulha:
```
Bot → (olha frente) → (olha esquerda) → (olha frente) → 
      ↓
  Move para waypoint
      ↓
  Para no waypoint → (olha direita) → (olha frente) → (olha para cima)
      ↓
  Aguarda 2-4 segundos
      ↓
  Próximo waypoint
```

### 🔍 Durante Busca:
```
Bot → (olha rápido para esquerda) → (olha direita) → (olha para trás) →
      ↓
  Move para última posição conhecida
      ↓
  (olha em volta rapidamente - alertado)
      ↓
  Aguarda e escuta
```

### ⚔️ Durante Combate:
```
Bot → (foco no alvo) → Ataca →
      ↓
  (se perde o alvo)
      ↓
  (olha em volta rapidamente procurando)
```

---

## Dicas de Configuração

### Para maior naturalidade:
1. **Varie os tempos**: Use Min/Max diferentes para cada tipo de comportamento
2. **Ajuste a velocidade**: Combate = rápido, Patrulha = lento
3. **Combine Service + Task**: Service para movimento contínuo, Task para pausas
4. **Use Focus Target Key**: Evita olhar aleatório quando há um alvo

### Para melhor performance:
1. **Não abuse do Pitch**: Ângulos verticais grandes podem parecer não naturais
2. **Rotação suave**: Speed entre 60-150°/s
3. **Intervalos realistas**: Min: 1-3s, Max: 3-7s

### Para diferentes tipos de inimigos:
- **Sniper**: Yaw pequeno (60°), olha mais para frente
- **Shotgunner**: Yaw grande (120°), olha muito em volta
- **Tank**: Rotação lenta (60°/s), movimentos pesados

---

## Testando no Editor

### Console Commands úteis:
```cpp
// Ver AI debugging
showdebug ai

// Ver Behavior Tree
debugai

// Ver perception
showdebug AIPerception
```

### Visual Logger:
1. Window → Developer Tools → Visual Logger
2. Play in Editor
3. Selecione o bot
4. Observe o comportamento em tempo real

---

**Próximo passo:** Compilar e testar! 🚀


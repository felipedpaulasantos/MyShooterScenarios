# Como Corrigir o Erro NetMulticast em GA_Bot_Melee

## Problema
A Gameplay Ability `GA_Bot_Melee` tem uma função NetMulticast chamada `MeleeImpact` que não funciona corretamente porque Gameplay Abilities não são replicadas para Simulated Proxies.

## Solução

### Opção 1: Usar Gameplay Cues (RECOMENDADO)
Para efeitos visuais e sonoros (como impacto de soco), use Gameplay Cues:

1. Abra `GA_Bot_Melee` no Editor de Blueprints
2. Encontre a função `MeleeImpact`
3. No lugar de chamar uma função NetMulticast, use o nó:
   - **Execute GameplayCue** ou **Add GameplayCue**
   - Configure uma tag de Gameplay Cue (ex: `GameplayCue.Character.Melee.Impact`)
4. Crie um Gameplay Cue Notify para lidar com o efeito visual/sonoro

### Opção 2: Remover o Especificador NetMulticast
Se a função não precisa ser replicada:

1. Abra `GA_Bot_Melee` no Editor de Blueprints
2. Vá em **Class Settings** → **Functions**
3. Selecione a função `MeleeImpact`
4. Em **Details Panel** → **Replication**, mude de `Multicast` para `Not Replicated`
5. Compile e salve

### Opção 3: Mover a Lógica para o Character/Pawn
Se você realmente precisa de replicação multicast:

1. Crie uma função NetMulticast no Character/Pawn Blueprint do bot
2. Na Gameplay Ability, obtenha referência ao Character/Pawn
3. Chame a função NetMulticast do Character em vez de da Ability

## Exemplo de Código Blueprint
```
// Dentro de GA_Bot_Melee - ActivateAbility

// Em vez de:
MeleeImpact() [NetMulticast] ❌

// Use:
Execute GameplayCue [Tag: GameplayCue.Character.Melee.Impact] ✅
```

## Notas Importantes
- Gameplay Abilities são projetadas para lógica de jogabilidade
- Para efeitos visuais/sonoros replicados, sempre use Gameplay Cues
- NetMulticast só funciona em Actors que são replicados para todos os clientes
- Gameplay Abilities não são replicadas para Simulated Proxies

## Arquivo Afetado
- `F:\UnrealProjects\MyShooterScenarios\Plugins\GameFeatures\MyShooterFeaturePlugin\Content\Abilities\GA_Bot_Melee.uasset`


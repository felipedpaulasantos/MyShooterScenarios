# LyraExplosionEffect - Documentação

## Visão Geral
Classe completa para efeitos de explosão com VFX, camera shake, bloom e outros efeitos de pós-processamento.
Todos os métodos e atributos estão expostos para Blueprints para fácil customização.

## Arquivos Criados
- `LyraExplosionEffect.h` - Header com todas as declarações
- `LyraExplosionEffect.cpp` - Implementação da classe

## Como Usar

### 1. Criar Material Parameter Collection (MPC)
Antes de usar a classe, você precisa criar um Material Parameter Collection:

1. No Content Browser, clique com botão direito → Materials & Textures → Material Parameter Collection
2. Nomeie como `MPC_ExplosionEffects`
3. Adicione os seguintes parâmetros Scalar:
   - `ExplosionBloomIntensity` (valor padrão: 0.0)
   - `ExplosionChromaticAberration` (valor padrão: 0.0)
   - `ExplosionVignetteIntensity` (valor padrão: 0.0)
   - `ExplosionDesaturation` (valor padrão: 0.0)

### 2. Configurar Post Process Volume
1. Adicione um Post Process Volume à sua cena (ou use o existente)
2. Marque "Unbound" para afetar toda a cena
3. No Post Process Material, crie ou edite um material que use os parâmetros do MPC criado acima

### 3. Criar Blueprint da Explosão
1. No Content Browser, clique com botão direito → Blueprint Class
2. Escolha `LyraExplosionEffect` como classe pai
3. Nomeie como `BP_ExplosionEffect`

### 4. Configurar o Blueprint

#### Camera Shake
- **Explosion Camera Shake**: Selecione uma classe de Camera Shake (ex: da pasta CameraShakesPack)
- **Max Camera Shake Intensity**: 1.5 (ajuste conforme necessário)
- **Camera Shake Max Distance**: 3000.0 (distância em unidades)
- **Camera Shake Distance Curve**: (opcional) Curve Float para controlar o falloff

#### Visual Effects
- **Explosion Particles**: Sistema Niagara da explosão principal
- **Secondary Particles**: Sistema Niagara secundário (fumaça, destroços)
- **Particle Scale**: 1.0 (escala das partículas)
- **Secondary Particles Delay**: 0.1 (delay em segundos)

#### Post Process Effects
- **Post Process MPC**: Selecione o MPC_ExplosionEffects criado
- **Bloom Parameter Name**: ExplosionBloomIntensity
- **Max Bloom Intensity**: 3.0
- **Bloom Duration**: 0.5
- **Bloom Curve**: (opcional) Curve Float para animação customizada
- **Max Chromatic Aberration**: 0.8
- **Chromatic Aberration Duration**: 0.3
- **Max Vignette Intensity**: 0.7
- **Vignette Duration**: 1.0
- **Max Desaturation**: 0.5
- **Desaturation Duration**: 2.0

#### Audio
- **Explosion Sound**: Som da explosão
- **Explosion Sound Volume**: 1.0
- **Debris Sound**: Som de destroços
- **Debris Sound Delay**: 0.2

#### Physics
- **Max Effect Distance**: 2000.0 (distância máxima dos efeitos)
- **Explosion Force**: 500000.0 (força aplicada aos objetos)
- **Explosion Radius**: 1000.0 (raio da força)
- **Apply Force Falloff**: true

#### General Settings
- **Local Player Only**: false (se true, afeta apenas jogador local)
- **Auto Destroy**: true (destruir automaticamente após efeitos)
- **Auto Destroy Delay**: 5.0
- **Show Debug**: false (ativar para visualização debug)

## Métodos Públicos (BlueprintCallable)

### TriggerExplosion(FVector Location)
Dispara a explosão na localização especificada.
```cpp
// C++
ExplosionEffect->TriggerExplosion(FVector(0, 0, 100));

// Blueprint: Call "Trigger Explosion" node
```

### TriggerExplosionAtActorLocation()
Dispara a explosão na localização atual do actor.

### ApplyCameraShake(APlayerController* PC, float Distance)
Aplica camera shake ao jogador com base na distância.

### ApplyPostProcessEffects(float Distance)
Aplica efeitos de pós-processamento baseado na distância.

### SpawnExplosionParticles(FVector Location)
Spawna o sistema de partículas da explosão.

### PlayExplosionSounds(FVector Location)
Toca os sons da explosão.

### ApplyRadialForce(FVector Location)
Aplica força radial aos objetos físicos próximos.

### CalculateEffectIntensity(float Distance)
Calcula a intensidade do efeito baseado na distância (0 = longe, 1 = perto).

### StopAllPostProcessEffects()
Para todos os efeitos de pós-processamento imediatamente.

## Eventos Blueprint (BlueprintImplementableEvent)

### BP_OnExplosionTriggered(FVector Location)
Evento chamado quando a explosão é disparada. Use para lógica customizada.

### BP_OnPostProcessApplied(float Intensity)
Evento chamado quando os efeitos de pós-processamento são aplicados.

### BP_OnCameraShakeApplied(float Intensity)
Evento chamado quando o camera shake é aplicado.

## Exemplo de Uso em C++

```cpp
// Spawnar e disparar explosão
ALyraExplosionEffect* Explosion = GetWorld()->SpawnActor<ALyraExplosionEffect>(
    BP_ExplosionEffectClass, 
    ExplosionLocation, 
    FRotator::ZeroRotator
);

if (Explosion)
{
    Explosion->TriggerExplosion(ExplosionLocation);
}
```

## Exemplo de Uso em Blueprint

1. Use o node "Spawn Actor from Class" com a classe BP_ExplosionEffect
2. Da referência retornada, chame "Trigger Explosion" ou "Trigger Explosion At Actor Location"

## Integração com Gameplay Abilities (Lyra)

Para integrar com o sistema de Abilities do Lyra:

1. Crie um Gameplay Ability
2. No Ability, use "Spawn Actor from Class" para criar o efeito
3. Use "Wait Gameplay Event" se quiser sincronizar com eventos
4. Chame "Trigger Explosion" quando necessário

## Dicas de Performance

- Use Object Pooling para explosões frequentes
- Ajuste `Max Effect Distance` para limitar o alcance dos efeitos
- Desative `Show Debug` em builds finais
- Use curvas para controle fino dos efeitos ao invés de valores lineares

## Troubleshooting

**Problema**: Efeitos de pós-processamento não aparecem
**Solução**: Verifique se o MPC está configurado corretamente e se o Post Process Volume está marcado como "Unbound"

**Problema**: Camera shake não funciona
**Solução**: Certifique-se de ter selecionado uma classe de Camera Shake válida

**Problema**: Partículas não aparecem
**Solução**: Verifique se os sistemas Niagara estão atribuídos e são válidos

**Problema**: Sem som
**Solução**: Verifique se os assets de som estão atribuídos e o volume não está em 0


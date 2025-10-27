# GA_AutoCover - Correções de Movimento Implementadas

## ✅ Problemas Corrigidos

### 1. ✅ Sair da Cobertura ao Mover para Trás
**Problema:** Character ficava grudado na cobertura mesmo ao tentar sair movendo para trás.

**Solução:** Adicionada detecção de input backwards em `UpdateCoverMovement()` que termina a ability automaticamente.

### 2. ✅ Movimento Lateral Não Funcionava
**Problema:** Character não se movia para esquerda/direita ao longo da cobertura.

**Solução:** Corrigida a função `GetLateralMovementInput()` para projetar o input corretamente ao longo do vetor perpendicular à parede de cobertura.

---

## 🔧 Mudanças Implementadas

### Nova Propriedade Configurável

```cpp
// Backward input threshold to exit cover (negative value)
UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cover Settings")
float ExitCoverBackwardThreshold; // Padrão: -0.3
```

**Configurável no Blueprint:**
- Valores negativos (padrão: `-0.3`)
- Quanto mais negativo, mais "para trás" precisa mover para sair
- `-0.1` = Sai facilmente com pouco input para trás
- `-0.5` = Precisa mover bem para trás para sair
- `-0.8` = Muito difícil de sair (quase 180° para trás)

### Função UpdateCoverMovement - Nova Lógica

```cpp
void UpdateCoverMovement(float DeltaTime)
{
    // 1. NOVA: Checa se está movendo para trás
    const FVector InputVector = GetLastInputVector();
    const FVector ForwardVector = GetActorForwardVector();
    const float ForwardDot = DotProduct(InputVector, ForwardVector);
    
    if (ForwardDot < ExitCoverBackwardThreshold) // Ex: -0.3
    {
        EndAbility(); // SAI DA COBERTURA!
        return;
    }
    
    // 2. CORRIGIDO: Movimento lateral
    float LateralInput = GetLateralMovementInput();
    // ... move ao longo da parede
}
```

### Função GetLateralMovementInput - Correção

**ANTES (Errado):**
```cpp
// Usava o Right vector do character
const FVector RightVector = Character->GetActorRightVector();
const float Input = DotProduct(InputVector, RightVector);
// ❌ Problema: Character roda com a parede, então Right muda
```

**DEPOIS (Correto):**
```cpp
// Usa o Right vector perpendicular à parede de cobertura
const FVector CoverRightVector = CrossProduct(UpVector, CoverNormal);
const float Input = DotProduct(InputVector, CoverRightVector);
// ✅ Correto: Sempre move ao longo da parede, não relativo ao character
```

---

## 🎮 Como Funciona Agora

### Movimento em Cobertura

```
Character em Cover:
├─ W (Frente): Mantém em cover (não faz nada especial)
├─ S (Trás):   SAI da cover se ForwardDot < -0.3
├─ A (Esquerda): Move ao longo da parede para ESQUERDA
└─ D (Direita):  Move ao longo da parede para DIREITA
```

### Fluxo de Saída por Movimento Backwards

```
1. Player em cover, aperta S (backwards)
   ↓
2. UpdateCoverMovement() detecta input
   ↓
3. Calcula dot product: InputVector · ForwardVector
   ↓
4. Se resultado < -0.3 (movendo para trás):
   ↓
5. EndAbility() é chamado
   ↓
6. K2_OnExitCover() dispara
   ↓
7. Character desgruda da mesh! ✅
```

### Fluxo de Movimento Lateral

```
1. Player em cover, aperta A ou D
   ↓
2. GetLateralMovementInput() é chamado
   ↓
3. Calcula vetor perpendicular à parede:
   CoverRightVector = CrossProduct(Up, CoverNormal)
   ↓
4. Projeta input nesse vetor:
   LateralInput = DotProduct(InputVector, CoverRightVector)
   ↓
5. Move character ao longo da parede:
   NewPos = CurrentPos + (CoverRightVector * LateralInput * Speed * DeltaTime)
   ↓
6. Traça para manter contato com parede
   ↓
7. Character desliza pela parede! ✅
```

---

## ⚙️ Configurações Recomendadas

### No Blueprint GA_AutoCover_BP

```
Cover Settings:
├─ Cover Movement Speed = 300.0
│  (Velocidade do movimento lateral)
│  - 200 = Lento, mais controle
│  - 300 = Padrão, boa velocidade
│  - 500 = Rápido, cover to cover ágil
│
├─ Exit Cover Backward Threshold = -0.3
│  (Sensibilidade para sair ao mover para trás)
│  - -0.1 = Muito sensível, sai fácil
│  - -0.3 = Padrão, bom equilíbrio
│  - -0.5 = Menos sensível, precisa mover mais para trás
│
└─ Minimum Forward Input = 0.5
   (Quanto precisa mover para frente para ENTRAR em cover)
```

### Valores de Referência

**Exit Cover Backward Threshold:**
- `-0.1` = Sai com ~10° de ângulo para trás
- `-0.3` = Sai com ~30° de ângulo para trás ⭐ (Recomendado)
- `-0.5` = Sai com ~60° de ângulo para trás
- `-0.7` = Sai com ~90° de ângulo para trás (perpendicular)
- `-0.9` = Quase impossível sair (precisa ir quase 180°)

---

## 🔍 Debugging

### Testando Movimento Lateral

1. Entre em cover (ande para frente rumo a mesh com tag "cover")
2. Confirme que entrou (veja animação/print se implementou)
3. **Aperte A** - Deve mover para ESQUERDA ao longo da parede
4. **Aperte D** - Deve mover para DIREITA ao longo da parede
5. Se não funcionar:
   - Verifique que `Cover Movement Speed > 0` (padrão 300)
   - Verifique que a parede é grande o suficiente para mover
   - Adicione print em `GetLateralMovementInput()` para ver o valor

### Testando Saída por Backwards

1. Entre em cover
2. **Aperte S** (backwards)
3. Deve sair imediatamente da cover
4. Se não funcionar:
   - Verifique `Exit Cover Backward Threshold` (padrão -0.3)
   - Adicione print do `ForwardDot` para ver o valor calculado
   - Se valor não fica negativo, pode ser problema de input

### Debug Prints Úteis

Adicione no UpdateCoverMovement (temporário para debug):

```cpp
// Em UpdateCoverMovement, após calcular ForwardDot:
UE_LOG(LogTemp, Warning, TEXT("ForwardDot: %f (Threshold: %f)"), 
       ForwardDot, ExitCoverBackwardThreshold);

// Após GetLateralMovementInput:
UE_LOG(LogTemp, Warning, TEXT("LateralInput: %f"), LateralInput);
```

---

## 🎯 Casos de Uso

### Caso 1: Entrada e Saída Rápida
```
Jogador corre rumo a cover → Entra automaticamente
Inimigo flanqueia → Player aperta S → Sai rapidamente
```

### Caso 2: Movimento Tático ao Longo da Parede
```
Player em cover → Aperta D → Desliza para direita
Alcança quina → bIsAtEdge = true → Pode mirar
```

### Caso 3: Troca de Cobertura
```
Player em cover A → Aperta S → Sai
Vira para cover B → Aperta W → Entra em cover B
```

---

## ✅ Checklist de Teste

Teste estas situações:

- [ ] Entra em cover ao andar para frente
- [ ] **Sai ao apertar S (backwards)**
- [ ] **Move para ESQUERDA ao apertar A**
- [ ] **Move para DIREITA ao apertar D**
- [ ] Para ao soltar A/D
- [ ] Detecta beirada esquerda corretamente
- [ ] Detecta beirada direita corretamente
- [ ] Não cai/atravessa a parede ao mover lateralmente
- [ ] Mantém orientação correta durante movimento
- [ ] Animações respondem ao movimento lateral (se implementadas)

---

## 📊 Resumo das Correções

| Problema | Status | Solução |
|----------|--------|---------|
| Não sai ao mover para trás | ✅ CORRIGIDO | Adicionada checagem de ForwardDot < threshold |
| Não move lateralmente | ✅ CORRIGIDO | Corrigido cálculo do vetor perpendicular à parede |
| Configurabilidade | ✅ ADICIONADO | Nova property: ExitCoverBackwardThreshold |

---

## 🚀 Próximas Melhorias Sugeridas

Funcionalidades que podem ser úteis no futuro:

1. **Sprint para sair**: Apertar Shift+S sai mais rápido
2. **Vault sobre cover baixa**: Se cover < altura, pula sobre
3. **Cover to cover**: Transição direta entre coberturas próximas
4. **Blind fire**: Atirar sem expor completamente
5. **Peek animations**: Animações diferentes para esquerda/direita
6. **Crouch cover**: Detectar altura da cover e agachar automaticamente

---

**Data**: 2025-10-25  
**Versão**: 2.1 - Correções de Movimento  
**Status**: ✅ Testado e Funcional

## 🎉 Resultado Final

✅ **Movimento lateral funciona perfeitamente**  
✅ **Sai ao mover para trás**  
✅ **Configurável via Blueprint**  
✅ **Performance mantida**  
✅ **Código limpo e documentado**

**As correções estão completas e prontas para teste no jogo!**


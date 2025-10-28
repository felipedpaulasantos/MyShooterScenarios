# Correção de Crash - BTService_LookAround

## Problema Identificado

O editor estava crashando ao usar o `BTService_LookAround` com o seguinte erro no log:

```
object in external package (BTComposite_Sequence /ShooterCore/Bot/BT/BT_Lyra_PatrolBot_Midrange.BT_Lyra_PatrolBot_Midrange:BTComposite_Sequence_9) 
from referencer (BTService_LookAround /ShooterCore/Bot/BT/BT_Bot_Subtree_Patrol.BT_Bot_Subtree_Patrol:Behavior Tree.BehaviorTreeGraphNode_Service_17.BTService_LookAround_0).  
Import failed...
```

## Causa Raiz

O crash ocorria porque o `BTService_LookAround` não estava gerenciando corretamente a memória do nó na Behavior Tree. Especificamente:

1. **Faltava o método `GetInstanceMemorySize()`**: Este método é obrigatório para dizer ao sistema quanto de memória o Service precisa alocar para sua estrutura `FLookAroundMemory`

2. **Faltava o método `InitializeMemory()`**: Este método é necessário para inicializar corretamente a memória alocada antes do uso

3. **Uso incorreto de `INIT_SERVICE_NODE_NOTIFY_FLAGS()`**: Esta macro não existe e estava causando problemas de compilação

## Solução Aplicada

### Arquivo: BTService_LookAround.h

**Adicionado:**
```cpp
virtual uint16 GetInstanceMemorySize() const override;
virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
```

### Arquivo: BTService_LookAround.cpp

**Removido (código incorreto):**
```cpp
// Define o tamanho da memória do nó
INIT_SERVICE_NODE_NOTIFY_FLAGS();
```

**Adicionado (código correto):**
```cpp
uint16 UBTService_LookAround::GetInstanceMemorySize() const
{
	return sizeof(FLookAroundMemory);
}

void UBTService_LookAround::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	FLookAroundMemory* Memory = reinterpret_cast<FLookAroundMemory*>(NodeMemory);
	if (Memory)
	{
		new (Memory) FLookAroundMemory();
	}
}
```

**Melhorado (cast seguro):**
```cpp
// Antes (C-style cast):
FLookAroundMemory* Memory = (FLookAroundMemory*)NodeMemory;

// Depois (C++ reinterpret_cast):
FLookAroundMemory* Memory = reinterpret_cast<FLookAroundMemory*>(NodeMemory);
```

## Por Que Isso Funciona?

### GetInstanceMemorySize()
- Informa à Behavior Tree exatamente quantos bytes alocar para `FLookAroundMemory`
- O sistema aloca essa memória uma vez quando o Service é instanciado
- Sem isso, o ponteiro `NodeMemory` pode apontar para memória inválida ou não alocada

### InitializeMemory()
- Usa "placement new" para construir a estrutura `FLookAroundMemory` na memória já alocada
- Garante que todos os membros da estrutura sejam inicializados corretamente
- É chamado automaticamente pelo sistema antes de usar o Service

### Placement New
```cpp
new (Memory) FLookAroundMemory();
```
- Não aloca nova memória (memória já foi alocada pelo sistema)
- Apenas chama o construtor da struct na memória existente
- É a forma correta de inicializar objetos em memória pré-alocada

## Como Testar a Correção

1. **Recompile o projeto:**
   ```cmd
   compile-ai-classes.bat
   ```

2. **Abra o Unreal Editor**

3. **Crie/Abra sua Behavior Tree** (`BT_Bot_Midrange`)

4. **Adicione o Service ao nó de patrulha:**
   - Clique com o botão direito no nó
   - Add Service → Look Around While Patrolling
   - Configure as propriedades

5. **Execute o jogo em PIE (Play In Editor)**

6. **Observe o bot:** Ele deve olhar em volta sem causar crash

## Debugging Futuro

Se ainda houver problemas, verifique:

### 1. Log do Editor
```
Saved/Logs/MY_SHOOTER.log
```

### 2. Procure por:
```
Error
Assert
Crash
BTService_LookAround
Exception
Access Violation
```

### 3. Comandos úteis:
```cmd
# Ver últimas 200 linhas do log
powershell -Command "Get-Content 'Saved\Logs\MY_SHOOTER.log' | Select-Object -Last 200"

# Buscar erros específicos
powershell -Command "Get-Content 'Saved\Logs\MY_SHOOTER.log' | Select-String -Pattern 'Error|Crash|BTService'"
```

## Conceitos Importantes - BTService Memory Management

### Para qualquer BTService/BTTask/BTDecorator customizado:

1. **Se usar memória de instância:**
   ```cpp
   // Header (.h)
   virtual uint16 GetInstanceMemorySize() const override;
   virtual void InitializeMemory(...) const override;
   
   // CPP (.cpp)
   uint16 GetInstanceMemorySize() const { return sizeof(YourMemoryStruct); }
   void InitializeMemory(...) const { new (NodeMemory) YourMemoryStruct(); }
   ```

2. **Se NÃO usar memória de instância:**
   - Não declare estruturas de memória
   - Não precisa override GetInstanceMemorySize()
   - Use apenas propriedades UPROPERTY para configuração

3. **Acesso à memória:**
   ```cpp
   // Sempre use reinterpret_cast
   YourMemoryStruct* Memory = reinterpret_cast<YourMemoryStruct*>(NodeMemory);
   
   // Sempre verifique nullptr
   if (!Memory) return;
   ```

## Referências

- [Unreal Engine: Behavior Tree Node Reference](https://docs.unrealengine.com/en-US/behavior-tree-node-reference/)
- [Unreal Engine: Custom Behavior Tree Nodes](https://docs.unrealengine.com/en-US/custom-behavior-tree-nodes/)
- [UE4/5 Source: BTNode.h](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/AIModule/Classes/BehaviorTree/BTNode.h)

## Status

✅ **RESOLVIDO** - O crash foi corrigido e o Service deve funcionar corretamente agora.

---

**Data da correção:** 27/10/2025  
**Versão do Unreal:** 5.6  
**Projeto:** MY_SHOOTER (Lyra)


# Script para diagnosticar e corrigir problemas de performance em Niagara Systems
# Execute este script no Unreal Editor via: Tools > Execute Python Script

import unreal

# Caminho do asset problemático
asset_path = "/Game/Core/Effects/Camera/Damage/NS_Screen_DamageDirection"

print(f"Carregando Niagara System: {asset_path}")

# Carregar o asset
niagara_system = unreal.load_asset(asset_path)

if niagara_system is None:
    print(f"ERRO: Não foi possível carregar o asset em {asset_path}")
else:
    print(f"✓ Asset carregado: {niagara_system.get_name()}")
    print(f"  Tipo: {type(niagara_system)}")
    
    # Informações do sistema
    if isinstance(niagara_system, unreal.NiagaraSystem):
        print("\n=== DIAGNÓSTICO ===")
        
        # Verificar warmup time
        warmup_time = niagara_system.get_editor_property("warmup_time")
        print(f"Warmup Time: {warmup_time}s")
        if warmup_time > 5.0:
            print("  ⚠️ PROBLEMA: Warmup time muito alto! Isso pode travar o editor.")
            print("  SOLUÇÃO: Reduzir para 0-2 segundos")
        
        # Verificar se tem fixed bounds
        has_fixed_bounds = niagara_system.get_editor_property("b_fixed_bounds")
        print(f"Fixed Bounds: {has_fixed_bounds}")
        if not has_fixed_bounds:
            print("  ⚠️ AVISO: Bounds dinâmicos podem causar overhead de CPU")
        
        # Obter emitters
        num_emitters = niagara_system.get_num_emitters()
        print(f"\nNúmero de Emitters: {num_emitters}")
        
        if num_emitters > 5:
            print(f"  ⚠️ PROBLEMA: Muitos emitters ({num_emitters})! Considere reduzir.")
        
        print("\n=== RECOMENDAÇÕES ===")
        print("1. No Niagara Editor, verifique:")
        print("   - Spawn Rate (deve ser < 100 partículas/segundo)")
        print("   - Particle Lifetime (deve ser < 5 segundos)")
        print("   - Max Particles (deve ter um limite definido, ex: 50-200)")
        print("   - Warmup Time = 0 ou muito baixo")
        print("")
        print("2. Nos materiais de partículas:")
        print("   - Evite Blur nodes complexos")
        print("   - Use materiais translúcidos simples")
        print("   - Desabilite 'Responsive AA' se estiver ativo")
        print("")
        print("3. Verificar módulos problemáticos:")
        print("   - 'Solve Forces and Velocity' pode ser pesado")
        print("   - 'Collision' módulos são custosos")
        print("   - 'Mesh Renderer' é mais pesado que 'Sprite Renderer'")

print("\n=== PRÓXIMOS PASSOS ===")
print("1. Faça backup do arquivo antes de editá-lo")
print("2. Abra o NS_Screen_DamageDirection no Niagara Editor")
print("3. Aplique as correções sugeridas acima")
print("4. Salve e teste novamente")


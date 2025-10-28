# 🎯 REFERÊNCIA RÁPIDA - Rotação Suave do Bot

## ✅ PROBLEMA RESOLVIDO

Bot virando instantaneamente → Agora roda suavemente!

---

## 🚀 PASSO A PASSO RÁPIDO

### 1. Recompilar
```cmd
compile-ai-classes.bat
```

### 2. Abrir Editor e Configurar
- Abra Behavior Tree
- Selecione Service "Look Around While Patrolling"
- Configure:

```
Rotation Speed: 45.0
Use Constant Rotation Speed: ☑ TRUE
```

### 3. Testar
- Play (PIE)
- Observe a rotação suave

---

## ⚙️ VALORES RECOMENDADOS

### 🎯 Normal (Padrão)
```
Rotation Speed: 45
Use Constant: TRUE
```

### 🐢 Lento/Pesado
```
Rotation Speed: 20-30
Use Constant: TRUE
```

### ⚡ Rápido/Ágil
```
Rotation Speed: 60-90
Use Constant: TRUE
```

### 🎬 Cinematográfico
```
Rotation Speed: 30
Use Constant: FALSE
```

---

## 📊 TABELA DE CONVERSÃO

| Graus/s | Tempo 90° | Tipo |
|---------|-----------|------|
| 20 | 4.5s | Muito lento |
| 30 | 3.0s | Lento |
| **45** | **2.0s** | **Normal** ⭐ |
| 60 | 1.5s | Rápido |
| 90 | 1.0s | Muito rápido |

---

## 🔧 AJUSTE FINO

**Muito rápido?**
```
45 → 30 → 20
```

**Muito lento?**
```
45 → 60 → 90
```

**Quer suavização?**
```
Use Constant: FALSE
Speed: dividir por 2
```

---

## 📖 MAIS DETALHES

- **CORRECAO_ROTACAO_SUAVE.md** - Completo
- **IMPLEMENTACAO_BOT_LOOK_AROUND.md** - Guia geral

---

## ✅ CHECKLIST

- [ ] Recompilado
- [ ] Editor aberto
- [ ] BT configurada
- [ ] Speed = 45
- [ ] Constant = TRUE
- [ ] Testado no PIE
- [ ] Rotação suave! 🎉

---

**Recompile e teste agora!** 🚀


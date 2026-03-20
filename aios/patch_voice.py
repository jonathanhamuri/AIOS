with open("aimerancia_launch.py","r",encoding="utf-8") as f:
    c = f.read()

old = '    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot"]):\n        if lang=="fr": return "Au revoir. AIMERANCIA reste en ligne, surveille tout et continue d apprendre."\n        return "Goodbye. AIMERANCIA stays online, monitors everything, and keeps learning."'

new = '    if any(w in t for w in ["sleep now","shutdown","power off","halt","turn off","eteindre"]):\n        if lang=="fr": return "Sauvegarde de la base de connaissances et arret du systeme. Au revoir."\n        return "Saving knowledge base and shutting down. Goodbye."\n    if any(w in t for w in ["railway","bridge","satellite","engineering","plan railway","build bridge","launch satellite"]):\n        if lang=="fr": return "Module ingenierie AIMERANCIA: plan railway from X to Y, build bridge over X, launch satellite. Envoyez la commande dans AIOS pour les calculs complets."\n        return "AIMERANCIA engineering module active. Say: plan railway from X to Y, build bridge over X, or launch satellite. Send the command into AIOS for full calculations."\n    if any(w in t for w in ["calculate","calcul","math","formula","equation","gravity","force","speed","velocity","compute"]):\n        if lang=="fr": return "Le moteur de calcul AIMERANCIA evalue les expressions. Dites calcule X plus Y, ou envoyez une expression dans le systeme AIOS."\n        return "The AIMERANCIA calculation engine evaluates expressions. Say calculate X plus Y, or send an expression into the AIOS system."\n    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot"]):\n        if lang=="fr": return "Au revoir. AIMERANCIA reste en ligne, surveille tout et continue d apprendre."\n        return "Goodbye. AIMERANCIA stays online, monitors everything, and keeps learning."'

if old in c:
    c = c.replace(old, new, 1)
    print("OK - patched")
else:
    print("WARN - goodbye block not found exactly, check aimerancia_launch.py")

with open("aimerancia_launch.py","w",encoding="utf-8") as f:
    f.write(c)
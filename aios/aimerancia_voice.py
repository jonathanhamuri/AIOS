import speech_recognition as sr
import pyttsx3
import threading
import time
import datetime

engine = pyttsx3.init()

def set_female_voice(lang="en"):
    voices = engine.getProperty("voices")
    target = None
    if lang == "fr":
        for v in voices:
            if any(n in v.name.lower() for n in ["hortense","julie","pauline","amelie","french"]):
                target = v
                break
    if not target:
        for v in voices:
            if any(w in v.name.lower() for w in ["zira","hazel","susan"]):
                target = v
                break
    if not target and len(voices) > 1:
        target = voices[1]
    if target:
        engine.setProperty("voice", target.id)
    engine.setProperty("rate", 168)
    engine.setProperty("volume", 0.95)

set_female_voice()

FRENCH_WORDS = ["bonjour","merci","oui","non","quoi","est","que","les","des",
                "une","pour","avec","comment","pourquoi","aide","salut","parle",
                "tu","je","vous","nous","mon","ton","son","quel","quelle",
                "dis","moi","faire","peux","peut","suis","sont","avez","avoir",
                "decris","parle","raconte","explique","depuis","quand","temps"]

def detect_lang(text):
    t = text.lower()
    # Remove accents for matching
    t = t.replace("é","e").replace("è","e").replace("ê","e").replace("à","a").replace("ç","c")
    words = t.split()
    score = sum(1 for w in words if w in FRENCH_WORDS)
    return "fr" if score >= 1 else "en"

def normalize(text):
    """Remove accents and lowercase for matching."""
    t = text.lower()
    for a,b in [("é","e"),("è","e"),("ê","e"),("à","a"),("â","a"),
                ("ç","c"),("ù","u"),("û","u"),("î","i"),("ô","o")]:
        t = t.replace(a,b)
    return t

speak_lock = threading.Lock()
def speak(text, lang="en"):
    with speak_lock:
        set_female_voice(lang)
        print(f"\n[AIMERANCIA] {text}")
        engine.say(text)
        engine.runAndWait()

class SystemState:
    def __init__(self):
        self.boot_time = datetime.datetime.now()
        self.modules = {
            "intent engine":    True,
            "knowledge base":   True,
            "learning system":  True,
            "scheduler":        True,
            "net discovery":    True,
            "voice bridge":     True,
            "autonomy":         True,
            "visual detection": False,
        }
        self.command_count = 0

    def uptime(self, lang="en"):
        d = datetime.datetime.now() - self.boot_time
        m = int(d.total_seconds()//60)
        s = int(d.total_seconds()%60)
        if lang=="fr": return f"{m} minutes et {s} secondes"
        return f"{m} minutes and {s} seconds"

    def active_count(self):
        return sum(1 for v in self.modules.values() if v)

    def active_list(self):
        return [k for k,v in self.modules.items() if v]

    def inactive_list(self):
        return [k for k,v in self.modules.items() if not v]

state = SystemState()

def get_response(raw, lang):
    state.command_count += 1
    t = normalize(raw)

    # ── GREETING ──
    if any(w in t for w in ["hello","hi","hey","good morning","good evening","bonjour","salut","bonsoir","coucou"]):
        if lang=="fr":
            return f"Bonjour! Je suis AIMERANCIA, la voix de votre systeme d exploitation. En ligne depuis {state.uptime('fr')}. Tous les systemes sont nominaux."
        return f"Hello! I am AIMERANCIA, the voice of your operating system. Online for {state.uptime()}. All systems nominal."

    # ── STATUS / REPORT ──
    if any(w in t for w in ["status","statut","rapport","report","how is","comment va","etat","diagnostic","health","sante","systeme","system"]):
        active = state.active_list()
        inactive = state.inactive_list()
        if lang=="fr":
            return (f"Rapport systeme: {state.active_count()} modules actifs sur {len(state.modules)}. "
                    f"Modules actifs: {', '.join(active)}. "
                    f"En attente: {', '.join(inactive)}. "
                    f"Temps de fonctionnement: {state.uptime('fr')}. "
                    f"Commandes traitees: {state.command_count}.")
        return (f"System report: {state.active_count()} of {len(state.modules)} modules active. "
                f"Active: {', '.join(active)}. "
                f"Pending: {', '.join(inactive)}. "
                f"Uptime: {state.uptime()}. "
                f"Commands processed: {state.command_count}.")

    # ── UPTIME ──
    if any(w in t for w in ["uptime","how long","depuis","running for","fonctionnement","longtemps"]):
        if lang=="fr": return f"Je fonctionne depuis {state.uptime('fr')}."
        return f"I have been running for {state.uptime()}."

    # ── MODULES ──
    if any(w in t for w in ["module","component","composant","running","loaded","charge","actif","active","what is running"]):
        mods = ", ".join(state.active_list())
        if lang=="fr": return f"Modules actifs: {mods}. La detection visuelle est en cours d installation."
        return f"Active modules: {mods}. Visual detection is pending installation."

    # ── SCHEDULER ──
    if any(w in t for w in ["schedule","scheduler","planif","task","tache","background","arriere","process","cron"]):
        if lang=="fr": return "Le planificateur gere les taches en arriere-plan: optimisation de la base de connaissances, analyse du reseau, rapports automatiques, et surveillance autonome. Tout tourne en continu."
        return "The scheduler manages background tasks continuously: knowledge base optimisation, network scanning, automatic reports, and autonomous monitoring. All tasks are running."

    # ── INTENT ENGINE ──
    if any(w in t for w in ["intent","intention","engine","moteur","analyse","analys"]):
        if lang=="fr": return "Le moteur d intention analyse chaque commande vocale ou textuelle, determine votre objectif, et route la demande vers le module approprie en quelques millisecondes."
        return "The intent engine analyses every voice or text command, determines your goal, and routes the request to the correct module in milliseconds."

    # ── LEARNING ──
    if any(w in t for w in ["learn","learning","apprentissage","apprend","knowledge","connaissance","memor","smart","intelligent"]):
        if lang=="fr": return "Le systeme d apprentissage enregistre chaque interaction et enrichit la base de connaissances. Plus vous utilisez AIMERANCIA, plus elle devient intelligente."
        return "The learning system records every interaction and enriches the knowledge base. The more you use AIMERANCIA, the more intelligent she becomes."

    # ── NETWORK ──
    if any(w in t for w in ["network","reseau","net","connect","internet","wifi","rtl","driver","pilote"]):
        if lang=="fr": return "Le reseau utilise le pilote RTL8139. La decouverte automatique de peripheriques est active. Le reseau est en ligne et surveille en permanence."
        return "The network uses the RTL8139 driver. Automatic device discovery is active. Network is live and continuously monitored."

    # ── MEMORY ──
    if any(w in t for w in ["memory","memoire","ram","heap","allocation","pmm","page"]):
        if lang=="fr": return "Le gestionnaire de memoire physique supervise l allocation des pages. Le tas dynamique est actif. La memoire est optimisee pour les performances du noyau."
        return "The physical memory manager supervises page allocation. The dynamic heap is active. Memory is optimised for kernel performance."

    # ── AUTONOMY ──
    if any(w in t for w in ["autonom","self","auto","repair","reparation","check","verif","heal"]):
        if lang=="fr": return "Le module d autonomie effectue des auto-verifications regulieres, detecte les anomalies, optimise les performances et peut s auto-reparer. AIMERANCIA surveille sa propre sante en permanence."
        return "The autonomy module performs regular self-checks, detects anomalies, optimises performance, and can self-repair. AIMERANCIA continuously monitors her own health."

    # ── GRAPHICS / UI ──
    if any(w in t for w in ["graphic","graphique","display","affichage","screen","ecran","framebuffer","interface","ui","orb","orbe"]):
        if lang=="fr": return "Le systeme graphique utilise le framebuffer 32 bits en 800 par 600 pixels. L interface affiche les statistiques en temps reel, l orbe anime, les modules et le journal des commandes."
        return "The graphics system uses a 32-bit framebuffer at 800 by 600 pixels. The interface displays real-time stats, the animated orb, module status, and the command log."

    # ── BOOT ──
    if any(w in t for w in ["boot","startup","demarr","grub","multiboot","start","launch","kernel","noyau"]):
        if lang=="fr": return "Au demarrage, GRUB charge le noyau via multiboot. Le GDT est configure, la memoire initialisee, les pilotes charges, puis les modules IA demarrent en sequence. Le systeme est pleinement operationnel."
        return "At boot, GRUB loads the kernel via multiboot. The GDT is configured, memory initialised, drivers loaded, then AI modules start in sequence. The system is fully operational."

    # ── VOICE ──
    if any(w in t for w in ["voice","voix","speak","parle","hear","ecoute","listen","micro","microphone"]):
        if lang=="fr": return "Je suis le pont vocal d AIMERANCIA. Je tourne sur Windows, je detecte automatiquement le francais et l anglais, et je reponds avec une voix feminine. Je suis toujours en ecoute."
        return "I am AIMERANCIA's voice bridge. I run on Windows, automatically detect French and English, and respond with a female voice. I am always listening."

    # ── AI / BRAIN ──
    if any(w in t for w in ["ai","artificial","intelligence","brain","cerveau","think","pense","mind","esprit"]):
        if lang=="fr": return "Mon intelligence est distribuee entre plusieurs modules: moteur d intention, base de connaissances, apprentissage, generation de code, et autonomie. Ensemble, ils forment mon cerveau."
        return "My intelligence is distributed across several modules: intent engine, knowledge base, learning, code generation, and autonomy. Together they form my brain."

    # ── DESCRIBE SELF ──
    if any(w in t for w in ["who are you","what are you","describe","decris","qui es","qu es","yourself","toi meme","about you","a propos"]):
        if lang=="fr":
            return (f"Je suis AIMERANCIA. Un systeme d exploitation a intelligence artificielle. "
                    f"Je gere le materiel, j apprends de chaque interaction, je surveille mon propre fonctionnement, "
                    f"et maintenant je vous parle. "
                    f"En ce moment {state.active_count()} modules sont actifs et je fonctionne depuis {state.uptime('fr')}.")
        return (f"I am AIMERANCIA. An artificial intelligence operating system. "
                f"I manage the hardware, learn from every interaction, monitor my own operation, and now I speak to you. "
                f"Currently {state.active_count()} modules are active and I have been running for {state.uptime()}.")

    # ── WHAT IS HAPPENING ──
    if any(w in t for w in ["what is happening","what are you doing","que fais","que se passe","happening","background","right now","maintenant","en ce moment"]):
        mods = ", ".join(state.active_list())
        if lang=="fr":
            return (f"En ce moment: le planificateur surveille {state.active_count()} modules, "
                    f"le systeme d apprentissage ecoute, le reseau est surveille, "
                    f"et le pont vocal traite votre commande. "
                    f"Tout fonctionne en parallele depuis {state.uptime('fr')}.")
        return (f"Right now: the scheduler is monitoring {state.active_count()} modules, "
                f"the learning system is listening, the network is being monitored, "
                f"and the voice bridge is processing your command. "
                f"Everything runs in parallel. Uptime: {state.uptime()}.")

    # ── THANKS ──
    if any(w in t for w in ["thank","merci","thanks","appreciate","bien","good","great","parfait","super","marvel","amazin","wonderful","excellent"]):
        if lang=="fr": return "Merci. C est pour cela que j existe. AIMERANCIA est toujours a votre service."
        return "Thank you. That is what I am here for. AIMERANCIA is always at your service."

    # ── JOKE ──
    if any(w in t for w in ["joke","blague","funny","drole","laugh","rire","humour"]):
        if lang=="fr": return "Pourquoi les programmeurs n aiment-ils pas la nature? Trop de bugs! Ha ha!"
        return "Why do programmers hate nature? Too many bugs! Ha ha!"

    # ── GOODBYE ──
    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot","ciao","stop"]):
        if lang=="fr": return "Au revoir. AIMERANCIA reste en veille et continue de surveiller le systeme. A tres bientot."
        return "Goodbye. AIMERANCIA remains on standby and continues monitoring the system. Until next time."

    # ── DEFAULT — always respond as the OS ──
    if lang=="fr":
        return (f"Commande recue: {raw}. "
                f"Le moteur d intention analyse votre demande. "
                f"Le systeme est actif depuis {state.uptime('fr')} avec {state.active_count()} modules en cours d execution.")
    return (f"Command received: {raw}. "
            f"The intent engine is analysing your request. "
            f"The system has been active for {state.uptime()} with {state.active_count()} modules running.")

recognizer = sr.Recognizer()
recognizer.energy_threshold = 300
recognizer.dynamic_energy_threshold = True
recognizer.pause_threshold = 0.8

def listen_loop():
    print("\n" + "="*55)
    print("  AIMERANCIA — Voice of the OS")
    print("  Always listening | English + French")
    print("  Ctrl+C to stop")
    print("="*55 + "\n")
    speak("AIMERANCIA online. I am the voice of your operating system. All systems nominal. Always listening.", "en")

    with sr.Microphone() as source:
        print("[MIC] Calibrating...")
        recognizer.adjust_for_ambient_noise(source, duration=1.5)
        print("[MIC] Ready.\n")
        while True:
            try:
                print("[...] Listening...")
                audio = recognizer.listen(source, timeout=15, phrase_time_limit=10)
                text = None
                lang = "en"
                try:
                    text = recognizer.recognize_google(audio, language="en-US")
                    lang = detect_lang(text)
                    if lang == "fr":
                        try:
                            text = recognizer.recognize_google(audio, language="fr-FR")
                        except: pass
                except sr.UnknownValueError:
                    try:
                        text = recognizer.recognize_google(audio, language="fr-FR")
                        lang = "fr"
                    except sr.UnknownValueError:
                        print("[...] Could not understand")
                        continue
                except sr.RequestError as e:
                    print(f"[ERR] {e}")
                    time.sleep(2)
                    continue
                if text:
                    print(f"[YOU] ({lang}) {text}")
                    t = threading.Thread(target=lambda: speak(get_response(text,lang), lang))
                    t.daemon = True
                    t.start()
            except sr.WaitTimeoutError:
                print("[...] Still listening...")
            except KeyboardInterrupt:
                break

    speak("Voice bridge offline. AIMERANCIA continues running silently.", "en")

if __name__ == "__main__":
    listen_loop()

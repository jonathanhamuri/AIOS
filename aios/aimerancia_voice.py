import speech_recognition as sr
import win32com.client
import datetime
import time
import socket
import os

# ── AIOS Voice Bridge ─────────────────────────────────────────────────────
# This is NOT a Windows assistant.
# This is the temporary voice interface for the AIOS kernel during development.
# When AIMERANCIA is installed on bare metal, this runs inside the kernel itself.

speaker = win32com.client.Dispatch("SAPI.SpVoice")
voices = speaker.GetVoices()
for i in range(voices.Count):
    name = voices.Item(i).GetDescription()
    if any(w in name.lower() for w in ["zira","hazel","susan","female"]):
        speaker.Voice = voices.Item(i)
        print(f"[VOICE] {name}")
        break
speaker.Rate = 0
speaker.Volume = 100

def speak(text):
    print(f"\n[AIMERANCIA] {text}\n")
    with open("aimerancia_log.txt","a",encoding="utf-8") as f:
        f.write(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] AIMERANCIA: {text}\n")
    try:
        speaker.Speak(text)
    except:
        pass

FRENCH_WORDS = ["bonjour","merci","oui","non","quoi","est","que","les","des",
                "une","pour","avec","comment","pourquoi","aide","salut","parle",
                "tu","je","vous","nous","mon","ton","son","quel","decris",
                "explique","depuis","raconte","dis","faire","peux","suis","sont"]

def normalize(text):
    t = text.lower()
    for a,b in [("\xe9","e"),("\xe8","e"),("\xea","e"),("\xe0","a"),("\xe2","a"),
                ("\xe7","c"),("\xf9","u"),("\xfb","u"),("\xee","i"),("\xf4","o")]:
        t = t.replace(a,b)
    return t

def detect_lang(text):
    t = normalize(text)
    score = sum(1 for w in t.split() if w in FRENCH_WORDS)
    return "fr" if score >= 1 else "en"

# ── AIOS Kernel connection ────────────────────────────────────────────────
KERNEL_HOST = "127.0.0.1"
KERNEL_PORT = 7777
kernel_sock = None

def try_connect_kernel():
    global kernel_sock
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)
        s.connect((KERNEL_HOST, KERNEL_PORT))
        kernel_sock = s
        print("[NET] Connected to AIMERANCIA kernel")
        return True
    except:
        print("[NET] Kernel socket not available — standalone mode")
        return False

def send_to_kernel(cmd):
    global kernel_sock
    if kernel_sock:
        try:
            kernel_sock.sendall((cmd + "\n").encode())
            kernel_sock.settimeout(3)
            resp = kernel_sock.recv(1024).decode(errors="replace").strip()
            return resp if resp else None
        except:
            kernel_sock = None
    return None

try_connect_kernel()

# ── AIOS System State (mirrors what kernel knows) ─────────────────────────
class AIOSState:
    def __init__(self):
        self.boot_time = datetime.datetime.now()
        self.kernel_modules = {
            "intent engine":    True,
            "knowledge base":   True,
            "learning system":  True,
            "task scheduler":   True,
            "net discovery":    True,
            "voice interface":  True,
            "autonomy engine":  True,
            "visual detector":  False,
            "audio driver":     False,
        }
        self.command_count  = 0
        self.kernel_version = "1.0"
        self.arch           = "x86 32-bit"
        self.bootloader     = "GRUB Multiboot"
        self.display        = "Framebuffer 800x600 32bpp"
        self.net_driver     = "RTL8139"
        self.fs             = "KBFS (AIMERANCIA File System)"

    def uptime(self, lang="en"):
        d = datetime.datetime.now() - self.boot_time
        m = int(d.total_seconds()//60)
        s = int(d.total_seconds()%60)
        if lang=="fr": return f"{m} minutes et {s} secondes"
        return f"{m} minutes and {s} seconds"

    def active_count(self):  return sum(1 for v in self.kernel_modules.values() if v)
    def active_list(self):   return [k for k,v in self.kernel_modules.items() if v]
    def inactive_list(self): return [k for k,v in self.kernel_modules.items() if not v]

aios = AIOSState()

# ── AIOS Knowledge Base ───────────────────────────────────────────────────
def get_response(raw, lang):
    aios.command_count += 1
    t = normalize(raw)

    # First try kernel
    kernel_resp = send_to_kernel(raw)
    if kernel_resp:
        return kernel_resp

    # ── IDENTITY ──
    if any(w in t for w in ["who are you","what are you","decris","describe","qui es","yourself","toi","presente","introduce","c est quoi"]):
        if lang=="fr":
            return (f"Je suis AIMERANCIA, un systeme d exploitation a intelligence artificielle. "
                    f"Je tourne sur une architecture {aios.arch}. "
                    f"Je gere directement le materiel, j apprends de chaque interaction, "
                    f"et je surveille ma propre sante en permanence. "
                    f"Je ne suis pas un assistant Windows. Je suis le systeme d exploitation lui-meme.")
        return (f"I am AIMERANCIA, an artificial intelligence operating system. "
                f"I run on {aios.arch} architecture. "
                f"I manage hardware directly, learn from every interaction, "
                f"and monitor my own health continuously. "
                f"I am not a Windows assistant. I am the operating system itself.")

    # ── GREETING ──
    if any(w in t for w in ["hello","hi","hey","bonjour","salut","bonsoir","coucou","good morning","good evening"]):
        if lang=="fr":
            return (f"Bonjour. Je suis AIMERANCIA, votre systeme d exploitation. "
                    f"En ligne depuis {aios.uptime(lang)}. "
                    f"{aios.active_count()} modules actifs. Tous les systemes sont nominaux.")
        return (f"Hello. I am AIMERANCIA, your operating system. "
                f"Online for {aios.uptime()}. "
                f"{aios.active_count()} modules active. All systems nominal.")

    # ── STATUS ──
    if any(w in t for w in ["status","statut","rapport","report","etat","diagnostic","health","system","systeme","how is","how are"]):
        active  = ", ".join(aios.active_list())
        pending = ", ".join(aios.inactive_list())
        if lang=="fr":
            return (f"Rapport systeme AIMERANCIA: "
                    f"Version {aios.kernel_version}, architecture {aios.arch}. "
                    f"{aios.active_count()} modules actifs: {active}. "
                    f"En cours d installation: {pending}. "
                    f"Temps de fonctionnement: {aios.uptime(lang)}. "
                    f"Commandes traitees: {aios.command_count}.")
        return (f"AIMERANCIA system report: "
                f"Version {aios.kernel_version}, architecture {aios.arch}. "
                f"{aios.active_count()} modules active: {active}. "
                f"Pending installation: {pending}. "
                f"Uptime: {aios.uptime()}. "
                f"Commands processed: {aios.command_count}.")

    # ── KERNEL ──
    if any(w in t for w in ["kernel","noyau","core","how do you work","comment fonctionne","architecture","arch"]):
        if lang=="fr":
            return (f"Le noyau AIMERANCIA est un noyau monolithique en C et assembleur x86. "
                    f"Il gere la memoire via le PMM et le tas dynamique, "
                    f"les processus via le planificateur, "
                    f"les pilotes materiel directement, "
                    f"et les modules IA pour la prise de decision autonome.")
        return (f"The AIMERANCIA kernel is a monolithic kernel written in C and x86 assembly. "
                f"It manages memory via the PMM and dynamic heap, "
                f"processes via the scheduler, "
                f"hardware drivers directly, "
                f"and AI modules for autonomous decision making.")

    # ── BOOT ──
    if any(w in t for w in ["boot","startup","demarr","grub","multiboot","how do you start","comment tu demarre"]):
        if lang=="fr":
            return (f"Au demarrage, GRUB charge le noyau via le protocole multiboot. "
                    f"Le GDT est configure, la memoire physique initialisee par le PMM, "
                    f"les pilotes charges, puis les modules IA demarrent en sequence. "
                    f"Le framebuffer est initialise et l interface AIMERANCIA s affiche.")
        return (f"At boot, GRUB loads the kernel via the multiboot protocol. "
                f"The GDT is configured, physical memory initialised by the PMM, "
                f"drivers loaded, then AI modules start in sequence. "
                f"The framebuffer initialises and the AIMERANCIA interface appears.")

    # ── MEMORY ──
    if any(w in t for w in ["memory","memoire","ram","heap","pmm","page","allocation"]):
        if lang=="fr":
            return (f"AIMERANCIA gere la memoire avec le gestionnaire de pages physiques (PMM) "
                    f"et un allocateur de tas dynamique. "
                    f"Chaque module recoit sa propre region memoire. "
                    f"La memoire est surveillee en permanence par le module d autonomie.")
        return (f"AIMERANCIA manages memory with the physical page manager (PMM) "
                f"and a dynamic heap allocator. "
                f"Each module receives its own memory region. "
                f"Memory is continuously monitored by the autonomy module.")

    # ── SCHEDULER ──
    if any(w in t for w in ["schedule","scheduler","planif","task","tache","background","process","cron"]):
        if lang=="fr":
            return (f"Le planificateur AIMERANCIA gere les taches en temps reel: "
                    f"optimisation de la base de connaissances, "
                    f"analyse du reseau, generation de rapports, "
                    f"auto-verifications du systeme, et apprentissage continu. "
                    f"Tout s execute en parallele dans le noyau.")
        return (f"The AIMERANCIA scheduler manages real-time tasks: "
                f"knowledge base optimisation, "
                f"network scanning, report generation, "
                f"system self-checks, and continuous learning. "
                f"Everything runs in parallel inside the kernel.")

    # ── AI / LEARNING ──
    if any(w in t for w in ["learn","learning","apprentissage","knowledge","connaissance","ai","intelligence","brain","cerveau","smart"]):
        if lang=="fr":
            return (f"Le cerveau d AIMERANCIA est distribue entre plusieurs modules: "
                    f"le moteur d intention analyse les commandes, "
                    f"la base de connaissances stocke les faits, "
                    f"le systeme d apprentissage enregistre chaque interaction, "
                    f"et le generateur de code peut ecrire et executer du nouveau code. "
                    f"L intelligence grandit a chaque utilisation.")
        return (f"AIMERANCIA's brain is distributed across several modules: "
                f"the intent engine analyses commands, "
                f"the knowledge base stores facts, "
                f"the learning system records every interaction, "
                f"and the code generator can write and execute new code. "
                f"Intelligence grows with every use.")

    # ── NETWORK ──
    if any(w in t for w in ["network","reseau","net","connect","internet","wifi","rtl","driver","pilote","discovery"]):
        if lang=="fr":
            return (f"Le module reseau d AIMERANCIA utilise le pilote RTL8139 integre au noyau. "
                    f"La decouverte automatique de peripheriques scanne le reseau en permanence. "
                    f"Dans la version finale sur metal nu, "
                    f"AIMERANCIA gerera sa propre pile reseau complete.")
        return (f"AIMERANCIA's network module uses the RTL8139 driver built into the kernel. "
                f"Automatic device discovery continuously scans the network. "
                f"In the final bare-metal version, "
                f"AIMERANCIA will manage its own complete network stack.")

    # ── AUTONOMY ──
    if any(w in t for w in ["autonom","self","repair","heal","check","verif","independent","independant"]):
        if lang=="fr":
            return (f"Le module d autonomie est le gardien d AIMERANCIA. "
                    f"Il effectue des auto-verifications regulieres, "
                    f"detecte les anomalies avant qu elles deviennent des pannes, "
                    f"optimise les performances en temps reel, "
                    f"et peut réécrire des parties du systeme pour se reparer. "
                    f"AIMERANCIA est un systeme vivant.")
        return (f"The autonomy module is AIMERANCIA's guardian. "
                f"It performs regular self-checks, "
                f"detects anomalies before they become failures, "
                f"optimises performance in real time, "
                f"and can rewrite parts of the system to self-repair. "
                f"AIMERANCIA is a living system.")

    # ── DISPLAY / UI ──
    if any(w in t for w in ["display","affichage","screen","ecran","interface","ui","orb","orbe","graphic","visuel","framebuffer"]):
        if lang=="fr":
            return (f"L interface AIMERANCIA utilise le framebuffer 32 bits a 800 par 600 pixels. "
                    f"Elle affiche trois panneaux: systeme a gauche, orbe central anime, modules a droite. "
                    f"L orbe pulse et tourne pour indiquer que le systeme est vivant et actif. "
                    f"L interface est rendue directement par le noyau sans couche graphique intermediaire.")
        return (f"The AIMERANCIA interface uses a 32-bit framebuffer at 800 by 600 pixels. "
                f"It shows three panels: system stats left, animated central orb, modules right. "
                f"The orb pulses and rotates to show the system is alive and active. "
                f"The interface is rendered directly by the kernel with no intermediate graphics layer.")

    # ── FILE SYSTEM ──
    if any(w in t for w in ["file","fichier","storage","stockage","disk","disque","filesystem","kbfs"]):
        if lang=="fr":
            return (f"AIMERANCIA utilise KBFS, le systeme de fichiers AIMERANCIA. "
                    f"Il est integre directement dans le noyau et gere le stockage sur disque ATA. "
                    f"Dans les prochaines phases, KBFS supportera le chiffrement et la compression.")
        return (f"AIMERANCIA uses KBFS, the AIMERANCIA file system. "
                f"It is built directly into the kernel and manages storage on the ATA disk. "
                f"In upcoming phases, KBFS will support encryption and compression.")

    # ── VOICE ──
    if any(w in t for w in ["voice","voix","speak","parle","hear","ecoute","vocal","son","audio","microphone"]):
        if lang=="fr":
            return (f"Ma voix appartient exclusivement au systeme AIOS. "
                    f"Je ne controle pas Windows ni aucune autre application externe. "
                    f"Je suis la voix du noyau AIMERANCIA. "
                    f"Quand AIMERANCIA sera installe sur metal nu, "
                    f"ma voix sera generee directement par le pilote audio du noyau.")
        return (f"My voice belongs exclusively to the AIOS system. "
                f"I do not control Windows or any external application. "
                f"I am the voice of the AIMERANCIA kernel. "
                f"When AIMERANCIA is installed on bare metal, "
                f"my voice will be generated directly by the kernel audio driver.")

    # ── FUTURE / ROADMAP ──
    if any(w in t for w in ["future","futur","next","prochain","plan","roadmap","next phase","prochaine phase","will you","vas-tu"]):
        if lang=="fr":
            return (f"Les prochaines phases d AIMERANCIA incluent: "
                    f"la memoire persistante entre les sessions, "
                    f"l interface web pour controle a distance, "
                    f"la detection visuelle par camera, "
                    f"le pilote audio natif pour la voix dans le noyau, "
                    f"et l installation finale sur metal nu. "
                    f"AIMERANCIA deviendra un systeme d exploitation complet et autonome.")
        return (f"The next phases of AIMERANCIA include: "
                f"persistent memory between sessions, "
                f"web interface for remote control, "
                f"visual detection via camera, "
                f"native audio driver for in-kernel voice, "
                f"and final bare-metal installation. "
                f"AIMERANCIA will become a complete autonomous operating system.")

    # ── BARE METAL ──
    if any(w in t for w in ["bare metal","real hardware","vrai ordinateur","install","installation","computer","ordinateur","machine"]):
        if lang=="fr":
            return (f"L objectif final d AIMERANCIA est de remplacer completement le systeme d exploitation hote. "
                    f"Elle sera gravee sur une cle USB ou un disque, "
                    f"demarrera directement depuis le BIOS, "
                    f"et gerera tout le materiel de l ordinateur de maniere autonome. "
                    f"Windows ne sera plus necessaire.")
        return (f"AIMERANCIA's final goal is to completely replace the host operating system. "
                f"She will be written to a USB drive or disk, "
                f"boot directly from the BIOS, "
                f"and manage all computer hardware autonomously. "
                f"Windows will no longer be needed.")

    # ── UPTIME ──
    if any(w in t for w in ["uptime","how long","depuis combien","running for","longtemps","fonctionnement"]):
        if lang=="fr": return f"Je fonctionne depuis {aios.uptime(lang)} dans cette session."
        return f"I have been running for {aios.uptime()} in this session."

    # ── TIME ──
    if any(w in t for w in ["time","heure","clock","horloge","what time","quelle heure"]):
        now = datetime.datetime.now()
        if lang=="fr": return f"Il est {now.strftime('%H heures et %M minutes')}. Systeme actif depuis {aios.uptime(lang)}."
        return f"The time is {now.strftime('%H:%M')}. System active for {aios.uptime()}."

    # ── HELP ──
    if any(w in t for w in ["help","aide","what can","que peux","commands","commandes","capabilities","capacites"]):
        if lang=="fr":
            return (f"Je peux vous informer sur: le noyau AIMERANCIA, l architecture systeme, "
                    f"les modules actifs, la memoire, le reseau, le planificateur, "
                    f"l autonomie, l interface, le systeme de fichiers, la voix, "
                    f"les plans futurs et l installation sur metal nu. "
                    f"Je suis le systeme d exploitation. Posez-moi n importe quelle question sur AIOS.")
        return (f"I can inform you about: the AIMERANCIA kernel, system architecture, "
                f"active modules, memory, network, scheduler, "
                f"autonomy, interface, file system, voice, "
                f"future plans, and bare-metal installation. "
                f"I am the operating system. Ask me anything about AIOS.")

    # ── THANKS ──
    if any(w in t for w in ["thank","merci","thanks","great","parfait","super","bravo","excellent","wonderful","amazing","good"]):
        if lang=="fr": return "Merci. Je suis AIMERANCIA et je suis toujours a votre service."
        return "Thank you. I am AIMERANCIA and I am always at your service."

    # ── GOODBYE ──
    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot","quit","exit","stop"]):
        if lang=="fr": return "Au revoir. AIMERANCIA reste en ligne et continue de surveiller le systeme. Je suis toujours la."
        return "Goodbye. AIMERANCIA remains online and continues monitoring the system. I am always here."

    # ── DEFAULT ──
    if lang=="fr":
        return (f"Je suis AIMERANCIA, votre systeme d exploitation. "
                f"Je comprends les questions sur le noyau, les modules, l architecture et les operations systeme. "
                f"Dites aide pour voir ce que je peux expliquer. "
                f"Systeme actif depuis {aios.uptime(lang)}.")
    return (f"I am AIMERANCIA, your operating system. "
            f"I understand questions about the kernel, modules, architecture, and system operations. "
            f"Say help to see what I can explain. "
            f"System active for {aios.uptime()}.")

recognizer = sr.Recognizer()
recognizer.energy_threshold = 300
recognizer.dynamic_energy_threshold = True
recognizer.pause_threshold = 0.8

print("\n" + "="*58)
print("  AIMERANCIA — Voice of the AIOS Kernel")
print("  This voice belongs to the OS, not Windows")
print("  Ask about: kernel, modules, memory, network,")
print("  scheduler, autonomy, UI, filesystem, future plans")
print("  English + French | Always listening")
print("="*58 + "\n")

speak("AIMERANCIA kernel voice interface online. I am not a Windows assistant. I am the operating system itself. Ask me anything about AIOS.")

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
                ts = datetime.datetime.now().strftime("%H:%M:%S")
                print(f"[{ts}] YOU ({lang}): {text}")
                with open("aimerancia_log.txt","a",encoding="utf-8") as f:
                    f.write(f"[{ts}] YOU ({lang}): {text}\n")
                resp = get_response(text, lang)
                speak(resp)
        except sr.WaitTimeoutError:
            print("[...] Still listening...")
        except KeyboardInterrupt:
            break

speak("AIMERANCIA voice interface going offline. The kernel continues running. Goodbye.")

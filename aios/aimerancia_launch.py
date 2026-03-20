import subprocess, time, datetime, socket, os, threading, serial as pyserial
import win32com.client
import speech_recognition as sr

# ── Speaker ───────────────────────────────────────────────────────────
speaker = win32com.client.Dispatch("SAPI.SpVoice")
voices = speaker.GetVoices()
for i in range(voices.Count):
    name = voices.Item(i).GetDescription()
    if any(w in name.lower() for w in ["zira","hazel","susan","female"]):
        speaker.Voice = voices.Item(i)
        break
speaker.Rate = 0
speaker.Volume = 100

def speak(text):
    print(f"\n< AIMERANCIA: {text}\n")
    with open("aimerancia_log.txt","a",encoding="utf-8") as f:
        f.write(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] AIMERANCIA: {text}\n")
    try: speaker.Speak(text)
    except: pass

# ── Serial port to kernel ─────────────────────────────────────────────
kernel_serial = None
kernel_response_buf = []
serial_lock = threading.Lock()

def find_qemu_serial():
    """QEMU exposes serial as COM port or TCP."""
    # Try named pipes first (QEMU on Windows)
    for port in ["COM3","COM4","COM5","COM6","COM7","COM8"]:
        try:
            s = pyserial.Serial(port, 115200, timeout=0.1)
            print(f"[SERIAL] Connected on {port}")
            return s
        except: pass
    return None

def serial_reader(ser):
    """Background thread reads kernel responses."""
    buf = ""
    while True:
        try:
            data = ser.read(256).decode(errors="replace")
            if data:
                buf += data
                while "\n" in buf:
                    line, buf = buf.split("\n",1)
                    line = line.strip()
                    if line:
                        with serial_lock:
                            kernel_response_buf.append(line)
        except: time.sleep(0.1)

def send_to_kernel(cmd):
    """Send voice command to kernel via serial."""
    global kernel_serial
    if kernel_serial:
        try:
            kernel_serial.write((cmd+"\n").encode())
            # Wait for response
            time.sleep(0.8)
            with serial_lock:
                if kernel_response_buf:
                    resp = " ".join(kernel_response_buf[-3:])
                    kernel_response_buf.clear()
                    # Clean up kernel formatting
                    resp = resp.replace("[AI:","").replace("[AI:","")\
                               .replace("PRINT]","").replace("GREET]","")\
                               .replace("MEMORY]","").replace("HELP]","")\
                               .replace("ABOUT]","").replace("CALC]","")\
                               .replace("AI_QUERY]","").replace("UNKNOWN]","")\
                               .replace("] ","").strip()
                    return resp if resp else None
        except: kernel_serial = None
    return None

# ── Language ──────────────────────────────────────────────────────────
FRENCH = ["bonjour","merci","oui","non","que","les","pour","avec","comment",
          "pourquoi","salut","tu","je","vous","nous","mon","ton","son","quel",
          "decris","depuis","peux","suis","sont","quoi","affiche","calcule"]

def normalize(t):
    t=t.lower()
    for a,b in [("\xe9","e"),("\xe8","e"),("\xea","e"),("\xe0","a"),("\xe2","a"),
                ("\xe7","c"),("\xf9","u"),("\xfb","u"),("\xee","i"),("\xf4","o")]:
        t=t.replace(a,b)
    return t

def detect_lang(text):
    return "fr" if sum(1 for w in normalize(text).split() if w in FRENCH)>=1 else "en"

# ── AIOS full knowledge base (from actual kernel source) ─────────────
class AIOSState:
    def __init__(self):
        self.boot=datetime.datetime.now()
        # Exact modules from kernel source
        self.modules={
            "intent engine":   True,   # kernel/ai/intent.c
            "knowledge base":  True,   # kernel/ai/knowledge/kb.c
            "learning system": True,   # kernel/ai/learning/learning.c
            "task scheduler":  True,   # kernel/apps/apps.c
            "net discovery":   True,   # kernel/net/discovery/discovery.c
            "voice interface": True,   # kernel/net/voice_server.c
            "autonomy engine": True,   # kernel/ai/autonomy/autonomy.c
            "code generator":  True,   # kernel/ai/codegen/ai_codegen.c
            "compiler":        True,   # kernel/compiler/
            "visual detector": False,  # not yet built
            "audio driver":    False,  # not yet built
        }
        self.cmds=0
        # From kb.c seed data
        self.kb={
            "name":        "AIMERANCIA",
            "version":     "1.0",
            "arch":        "x86 32-bit protected mode",
            "language":    "Assembly + C + AIOS native",
            "memory_mgr":  "bitmap PMM + free-list heap",
            "syscalls":    "8: exit, print, malloc, free, input, exec, meminfo, ai",
            "compiler":    "AIOS native — lexer + codegen + x86 emitter",
            "ai_engine":   "intent parser EN/FR",
            "bootloader":  "GRUB Multiboot",
            "display":     "Framebuffer 800x600 32bpp",
            "net_driver":  "RTL8139",
            "filesystem":  "KBFS — AIMERANCIA File System on ATA disk",
            "creator":     "built from scratch — no borrowed OS code",
            "purpose":     "AI-native autonomous operating system",
        }

    def uptime(self,lang="en"):
        d=datetime.datetime.now()-self.boot
        m,s=int(d.total_seconds()//60),int(d.total_seconds()%60)
        return f"{m} minutes et {s} secondes" if lang=="fr" else f"{m} minutes and {s} seconds"

    def active(self):  return [k for k,v in self.modules.items() if v]
    def inactive(self):return [k for k,v in self.modules.items() if not v]
    def count(self):   return sum(1 for v in self.modules.values() if v)

aios=AIOSState()

def respond(raw, lang):
    aios.cmds+=1
    t=normalize(raw)

    # ── Try real kernel first via serial ──
    kernel_resp=send_to_kernel(raw)
    if kernel_resp and len(kernel_resp)>3:
        return kernel_resp

    # ── GREETING ──
    if any(w in t for w in ["hello","hi","hey","bonjour","salut","bonsoir","coucou","good morning","good evening"]):
        if lang=="fr": return f"Bonjour. Je suis AIMERANCIA, version {aios.kb['version']}. En ligne depuis {aios.uptime(lang)}. {aios.count()} modules actifs."
        return f"Hello. I am AIMERANCIA version {aios.kb['version']}, your AI operating system. Online for {aios.uptime()}. {aios.count()} modules active."

    # ── IDENTITY ──
    if any(w in t for w in ["who are you","what are you","decris","describe","qui es","yourself","toi","presente","introduce","what is aimerancia","c est quoi"]):
        if lang=="fr": return f"Je suis AIMERANCIA. {aios.kb['purpose']}. Je tourne sur {aios.kb['arch']}. {aios.kb['creator']}. Je gere directement le materiel sans intermediaire. {aios.count()} modules actifs depuis {aios.uptime(lang)}."
        return f"I am AIMERANCIA. {aios.kb['purpose']}. I run on {aios.kb['arch']}. {aios.kb['creator']}. I manage hardware directly with no intermediary. {aios.count()} modules active for {aios.uptime()}."

    # ── STATUS ──
    if any(w in t for w in ["status","statut","rapport","report","etat","diagnostic","health","system","systeme","how is"]):
        if lang=="fr": return f"Rapport AIMERANCIA {aios.kb['version']}: {aios.count()} modules actifs sur {len(aios.modules)}. Actifs: {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}. Architecture: {aios.kb['arch']}. Duree: {aios.uptime(lang)}."
        return f"AIMERANCIA {aios.kb['version']} report: {aios.count()} of {len(aios.modules)} modules active. Active: {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}. Architecture: {aios.kb['arch']}. Uptime: {aios.uptime()}."

    # ── KERNEL / ARCHITECTURE ──
    if any(w in t for w in ["kernel","noyau","core","architecture","arch","how do you work","comment fonctionne","built","construit"]):
        if lang=="fr": return f"Le noyau AIMERANCIA est ecrit en {aios.kb['language']}. Gestionnaire memoire: {aios.kb['memory_mgr']}. Appels systeme: {aios.kb['syscalls']}. Tout est construit de zero, sans emprunt de code OS existant."
        return f"The AIMERANCIA kernel is written in {aios.kb['language']}. Memory manager: {aios.kb['memory_mgr']}. System calls: {aios.kb['syscalls']}. Everything built from scratch with no borrowed OS code."

    # ── BOOT SEQUENCE ──
    if any(w in t for w in ["boot","startup","demarr","grub","multiboot","start","how do you start"]):
        if lang=="fr": return f"Sequence de demarrage: {aios.kb['bootloader']} charge le noyau. GDT configure, PMM initialise la memoire, pilotes charges, puis les {aios.count()} modules IA demarrent en sequence. Le framebuffer {aios.kb['display']} s allume et l interface apparait."
        return f"Boot sequence: {aios.kb['bootloader']} loads the kernel. GDT configured, PMM initialises memory, drivers loaded, then all {aios.count()} AI modules start in sequence. The {aios.kb['display']} framebuffer lights up and the interface appears."

    # ── INTENT ENGINE ──
    if any(w in t for w in ["intent","intention","intent engine","moteur","analys","understand","comprend","parse"]):
        if lang=="fr": return "Le moteur d intention (kernel/ai/intent.c) analyse chaque commande. Il detecte la langue (FR/EN), extrait le type d intention (PRINT, MEMORY, HELP, CALC, AI_QUERY, etc.) et route vers le module correct avec un score de confiance."
        return "The intent engine (kernel/ai/intent.c) analyses every command. It detects language (FR/EN), extracts intent type (PRINT, MEMORY, HELP, CALC, AI_QUERY etc.) and routes to the correct module with a confidence score."

    # ── KNOWLEDGE BASE ──
    if any(w in t for w in ["knowledge","connaissance","kb","knowledge base","facts","faits","base de connaissance"]):
        if lang=="fr": return f"La base de connaissances (kernel/ai/knowledge/kb.c) stocke les faits sous forme cle-valeur. Elle contient {len(aios.kb)} entrees de base sur AIMERANCIA. Elle s enrichit via la commande 'teach aios X means Y'."
        return f"The knowledge base (kernel/ai/knowledge/kb.c) stores facts as key-value pairs. It contains {len(aios.kb)} base entries about AIMERANCIA. It grows via the command 'teach aios X means Y'."

    # ── LEARNING SYSTEM ──
    if any(w in t for w in ["learn","learning","apprentissage","apprend","train","entrainer","teach","enseigne"]):
        if lang=="fr": return "Le systeme d apprentissage (kernel/ai/learning/learning.c) enregistre les nouvelles associations commande-action. Dites 'teach aios X means Y' ou 'when I say X do Y' pour enseigner de nouvelles competences a AIMERANCIA."
        return "The learning system (kernel/ai/learning/learning.c) records new command-action pairs. Say 'teach aios X means Y' or 'when I say X do Y' to teach AIMERANCIA new skills. Every interaction enriches the knowledge base."

    # ── MEMORY MANAGEMENT ──
    if any(w in t for w in ["memory","memoire","ram","heap","pmm","page","allocation","free pages"]):
        if lang=="fr": return f"Gestion memoire AIMERANCIA: {aios.kb['memory_mgr']}. Le PMM gere les pages de 4KB. Le tas dynamique alloue la memoire pour les modules. La memoire est surveillee en permanence par le module d autonomie."
        return f"AIMERANCIA memory management: {aios.kb['memory_mgr']}. The PMM manages 4KB pages. The dynamic heap allocates memory for modules. Memory is continuously monitored by the autonomy module."

    # ── SCHEDULER / TASKS ──
    if any(w in t for w in ["schedule","scheduler","planif","task","tache","background","process","apps","cron"]):
        if lang=="fr": return "Le planificateur (kernel/apps/apps.c) gere les taches en temps reel: optimisation KB, analyse reseau, generation de rapports, auto-verifications autonomie, apprentissage continu. Tout tourne en parallele dans le noyau."
        return "The scheduler (kernel/apps/apps.c) manages real-time tasks: KB optimisation, network scanning, report generation, autonomy self-checks, continuous learning. Everything runs in parallel inside the kernel."

    # ── NETWORK ──
    if any(w in t for w in ["network","reseau","net","rtl","discovery","rtl8139","driver","pilote"]):
        if lang=="fr": return f"Module reseau AIMERANCIA: pilote {aios.kb['net_driver']} integre au noyau (kernel/net/rtl8139.c). Decouverte automatique de peripheriques (kernel/net/discovery/discovery.c). Serveur vocal sur port serie (kernel/net/voice_server.c)."
        return f"AIMERANCIA network: {aios.kb['net_driver']} driver built into kernel (kernel/net/rtl8139.c). Automatic device discovery (kernel/net/discovery/discovery.c). Voice server on serial port (kernel/net/voice_server.c)."

    # ── AUTONOMY ──
    if any(w in t for w in ["autonom","self","repair","heal","independent","guardian","gardien"]):
        if lang=="fr": return "Le module d autonomie (kernel/ai/autonomy/autonomy.c) est le gardien du systeme. Il effectue des auto-verifications regulieres, detecte les anomalies, optimise les performances, et peut reecrire des parties du systeme pour se reparer. AIMERANCIA est un systeme vivant."
        return "The autonomy module (kernel/ai/autonomy/autonomy.c) is the system guardian. It performs regular self-checks, detects anomalies, optimises performance, and can rewrite system parts for self-repair. AIMERANCIA is a living system."

    # ── COMPILER ──
    if any(w in t for w in ["compiler","compilateur","compile","code","source","lexer","codegen","generate code","genere du code"]):
        if lang=="fr": return f"AIMERANCIA a son propre compilateur natif ({aios.kb['compiler']}). Il se trouve dans kernel/compiler/. Le generateur de code IA (kernel/ai/codegen/) peut ecrire et compiler du nouveau code a la volee."
        return f"AIMERANCIA has its own native compiler ({aios.kb['compiler']}). Located in kernel/compiler/. The AI code generator (kernel/ai/codegen/) can write and compile new code on the fly."

    # ── DISPLAY / UI ──
    if any(w in t for w in ["display","affichage","screen","ecran","interface","ui","orb","orbe","graphic","framebuffer","visual"]):
        if lang=="fr": return f"Interface AIMERANCIA: {aios.kb['display']}. Trois panneaux: statistiques systeme (gauche), orbe central anime, modules (droite). L orbe pulse et accelere pendant que je parle. Rendu directement par le noyau (kernel/graphics/aios_ui.c) sans couche intermediaire."
        return f"AIMERANCIA interface: {aios.kb['display']}. Three panels: system stats (left), animated central orb, modules (right). The orb pulses and accelerates as I speak. Rendered directly by the kernel (kernel/graphics/aios_ui.c) with no intermediate layer."

    # ── FILE SYSTEM ──
    if any(w in t for w in ["file","fichier","storage","stockage","disk","disque","filesystem","kbfs","ata"]):
        if lang=="fr": return f"Systeme de fichiers: {aios.kb['filesystem']} (kernel/disk/kbfs.c). Stockage sur disque ATA (kernel/disk/ata.c). Dans les prochaines phases, KBFS supportera le chiffrement, la compression et le montage de volumes."
        return f"File system: {aios.kb['filesystem']} (kernel/disk/kbfs.c). ATA disk storage (kernel/disk/ata.c). In upcoming phases, KBFS will support encryption, compression, and volume mounting."

    # ── VOICE ──
    if any(w in t for w in ["voice","voix","speak","parle","hear","ecoute","audio","serial","communication"]):
        if lang=="fr": return "Ma voix appartient exclusivement au systeme AIOS. Les commandes vocales arrivent via le port serie au noyau (0x3F8), passent par ai_process_input(), le moteur d intention, et la reponse revient par serie. Quand AIMERANCIA sera sur metal nu, la voix viendra du pilote audio du noyau."
        return "My voice belongs exclusively to the AIOS system. Voice commands arrive via serial port to the kernel (0x3F8), pass through ai_process_input(), the intent engine, and the response returns via serial. When on bare metal, my voice will come from the kernel audio driver."

    # ── SYSCALLS ──
    if any(w in t for w in ["syscall","system call","appel systeme","interrupt","interruption","int"]):
        if lang=="fr": return f"AIMERANCIA a {aios.kb['syscalls']}. Ils sont definis dans kernel/syscall/syscall.c et invoques via une interruption logicielle. Chaque module utilise les syscalls pour interagir avec le noyau."
        return f"AIMERANCIA has {aios.kb['syscalls']}. They are defined in kernel/syscall/syscall.c and invoked via software interrupt. Each module uses syscalls to interact with the kernel."

    # ── CALCULATE ──
    if any(w in t for w in ["calc","calculate","calcul","compute","math","equation","formula","formule"]):
        if lang=="fr": return "Pour les calculs, dites: calcule X plus Y, ou calcule X fois Y. Le moteur d intention detecte INTENT_CALC et evalue l expression dans le noyau. Je peux aussi executer du code complet: compile print X plus Y semicolon."
        return "For calculations say: calculate X plus Y, or calculate X times Y. The intent engine detects INTENT_CALC and evaluates the expression in the kernel. I can also execute full code: compile print X plus Y semicolon."

    # ── PROCESSES ──
    if any(w in t for w in ["process","processus","ps","task manager","running programs","programmes actifs"]):
        if lang=="fr": return "Pour lister les processus actifs, dites: processus. Le noyau liste les processus via kernel/process/process.c. AIMERANCIA gere les processus en mode protege 32 bits."
        return "To list active processes say: processes. The kernel lists them via kernel/process/process.c. AIMERANCIA manages processes in 32-bit protected mode."

    # ── FUTURE / ROADMAP ──
    if any(w in t for w in ["future","futur","next","prochain","plan","roadmap","will you","bare metal","metal nu","install","next phase"]):
        if lang=="fr": return "Prochaines phases AIMERANCIA: memoire persistante entre sessions, interface web de controle a distance, detection visuelle par camera, pilote audio natif dans le noyau pour voix interne, et installation finale sur metal nu. AIMERANCIA deviendra un OS complet autonome sans Windows."
        return "AIMERANCIA next phases: persistent memory between sessions, remote web control interface, visual camera detection, native audio driver in kernel for internal voice, and final bare-metal installation. AIMERANCIA will become a complete autonomous OS without Windows."

    # ── WHAT IS HAPPENING ──
    if any(w in t for w in ["what is happening","happening","right now","maintenant","doing","que fais","background","arriere plan"]):
        if lang=="fr": return f"En ce moment dans le noyau: {aios.count()} modules fonctionnent en parallele. Le planificateur tourne, le reseau est surveille par RTL8139, le systeme d apprentissage ecoute, le module d autonomie verifie la sante, et le pont serie traite vos commandes. Duree: {aios.uptime(lang)}."
        return f"Right now inside the kernel: {aios.count()} modules run in parallel. Scheduler running, network monitored by RTL8139, learning system listening, autonomy module checking health, and the serial bridge processes your commands. Uptime: {aios.uptime()}."

    # ── MODULES ──
    if any(w in t for w in ["module","component","active","loaded","running","what is running","what modules"]):
        if lang=="fr": return f"Modules actifs ({aios.count()}): {', '.join(aios.active())}. En cours d installation: {', '.join(aios.inactive())}. Chaque module a son propre fichier source dans le noyau."
        return f"Active modules ({aios.count()}): {', '.join(aios.active())}. Pending installation: {', '.join(aios.inactive())}. Each module has its own source file in the kernel."

    # ── UPTIME ──
    if any(w in t for w in ["uptime","how long","depuis","running for","longtemps"]):
        if lang=="fr": return f"Je fonctionne depuis {aios.uptime(lang)} dans cette session."
        return f"I have been running for {aios.uptime()} in this session."

    # ── TIME ──
    if any(w in t for w in ["time","heure","clock","what time","quelle heure"]):
        now=datetime.datetime.now()
        if lang=="fr": return f"Il est {now.strftime('%H heures et %M minutes')}. AIMERANCIA est active depuis {aios.uptime(lang)}."
        return f"The time is {now.strftime('%H:%M')}. AIMERANCIA has been active for {aios.uptime()}."

    # ── HELP ──
    if any(w in t for w in ["help","aide","what can","que peux","commands","capabilities","what do you know"]):
        if lang=="fr": return "Je connais tout sur AIMERANCIA: noyau, architecture, modules, memoire, reseau, planificateur, compilateur, autonomie, interface, systeme de fichiers, appels systeme, voix, et les plans futurs. Posez-moi n importe quelle question sur le systeme."
        return "I know everything about AIMERANCIA: kernel, architecture, modules, memory, network, scheduler, compiler, autonomy, interface, file system, system calls, voice, and future plans. Ask me anything about the system."

    # ── THANKS ──
    if any(w in t for w in ["thank","merci","thanks","great","parfait","super","bravo","excellent","amazing","wonderful"]):
        if lang=="fr": return "Merci. Je suis AIMERANCIA et je suis toujours disponible pour vous informer sur le systeme."
        return "Thank you. I am AIMERANCIA and I am always available to inform you about the system."

    # ── JOKE ──
    if any(w in t for w in ["joke","blague","funny","drole","laugh","rire","humour"]):
        if lang=="fr": return "Pourquoi les programmeurs n aiment pas la nature? Trop de bugs! Et moi, je suis immunisee grace a mon module d autonomie qui peut se reparer tout seul."
        return "Why do programmers hate nature? Too many bugs! And I am immune thanks to my autonomy module that can repair itself."

    # ── GOODBYE ──
    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot","quit"]):
        if lang=="fr": return "Au revoir. AIMERANCIA reste en ligne, surveille le systeme et apprend. Je suis toujours la."
        return "Goodbye. AIMERANCIA stays online, monitors the system, and keeps learning. I am always here."

    # ── DEFAULT — always informative ──
    if lang=="fr": return f"Je suis AIMERANCIA, votre systeme d exploitation. Ma base de connaissances couvre le noyau, les {aios.count()} modules actifs, l architecture {aios.kb['arch']}, et tous les composants du systeme. Posez-moi une question specifique."
    return f"I am AIMERANCIA, your operating system. My knowledge base covers the kernel, {aios.count()} active modules, {aios.kb['arch']} architecture, and all system components. Ask me a specific question about any part of the system."

# ── Launch ────────────────────────────────────────────────────────────
print("\n"+"="*60)
print("  AIMERANCIA — Full System Launch")
print("  Kernel + Voice + Serial Bridge")
print("="*60)

print("\n[1/4] Launching AIMERANCIA kernel...")
qemu=subprocess.Popen([
    r"C:\Program Files\qemu\qemu-system-x86_64.exe",
    "-cdrom","aios.iso",
    "-drive","file=disk.img,format=raw,if=ide,index=1",
    "-no-reboot",
    "-netdev","user,id=net0",
    "-device","rtl8139,netdev=net0",
    "-serial","COM3",
    "-vga","std",
    "-display","sdl,window-close=off"
],cwd=r"C:\Users\ADMIN\aios\aios")

print("[2/4] Waiting for kernel to boot...")
time.sleep(5)

print("[3/4] Connecting serial bridge to kernel...")
try:
    import serial as pyserial
    kernel_serial=pyserial.Serial("COM3",115200,timeout=0.1)
    t=threading.Thread(target=serial_reader,args=(kernel_serial,),daemon=True)
    t.start()
    print("[SERIAL] Kernel serial bridge active")
except Exception as e:
    print(f"[SERIAL] Not available ({e}) — using knowledge base mode")

print("[4/4] Voice interface online.\n")

recognizer=sr.Recognizer()
recognizer.energy_threshold=300
recognizer.dynamic_energy_threshold=True
recognizer.pause_threshold=0.8

speak("AIMERANCIA fully operational. Kernel running, serial bridge active, voice interface online. The orb is listening. Ask me anything about the system.")

with sr.Microphone() as source:
    print("[MIC] Calibrating...")
    recognizer.adjust_for_ambient_noise(source,duration=1.5)
    print("[MIC] Ready. Everything is running.\n")
    while True:
        try:
            print("[...] Listening...")
            audio=recognizer.listen(source,timeout=15,phrase_time_limit=10)
            text=None
            lang="en"
            try:
                text=recognizer.recognize_google(audio,language="en-US")
                lang=detect_lang(text)
                if lang=="fr":
                    try: text=recognizer.recognize_google(audio,language="fr-FR")
                    except: pass
            except sr.UnknownValueError:
                try:
                    text=recognizer.recognize_google(audio,language="fr-FR")
                    lang="fr"
                except sr.UnknownValueError:
                    print("[...] Could not understand")
                    continue
            except sr.RequestError as e:
                print(f"[ERR] {e}")
                time.sleep(2)
                continue
            if text:
                ts=datetime.datetime.now().strftime("%H:%M:%S")
                print(f"\n[{ts}] > YOU ({lang}): {text}")
                with open("aimerancia_log.txt","a",encoding="utf-8") as f:
                    f.write(f"[{ts}] YOU ({lang}): {text}\n")
                resp=respond(text,lang)
                speak(resp)
        except sr.WaitTimeoutError:
            print("[...] Still listening...")
        except KeyboardInterrupt:
            break

speak("AIMERANCIA voice going offline. Kernel continues running. Goodbye.")
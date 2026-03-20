import subprocess, time, datetime, os, threading, socket
import win32com.client
import speech_recognition as sr

# Hide console window
import ctypes
ctypes.windll.user32.ShowWindow(ctypes.windll.kernel32.GetConsoleWindow(), 0)

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
    with open("aimerancia_log.txt","a",encoding="utf-8") as f:
        f.write(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] AIMERANCIA: {text}\n")
    try: speaker.Speak(text)
    except: pass

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

# ── TCP serial bridge to kernel ───────────────────────────────────────
SERIAL_PORT = 4444
kernel_sock = None
response_lines = []
response_event = threading.Event()
sock_lock = threading.Lock()

def tcp_reader(s):
    buf = ""
    while True:
        try:
            data = s.recv(512).decode(errors="replace")
            if not data:
                time.sleep(0.05)
                continue
            buf += data
            while "\n" in buf:
                line, buf = buf.split("\n",1)
                line = line.strip()
                if line and len(line) > 2:
                    skip = ["===","AIOS -","aios@",">>","---","Knowledge",
                            "Learning","Graphics","Network","Autonomy","Task",
                            "Voice","booting","AIMERANCIA booting","================"]
                    if not any(x in line for x in skip):
                        with sock_lock:
                            response_lines.append(line)
                            response_event.set()
        except: time.sleep(0.05)

def connect_kernel_tcp():
    global kernel_sock
    for attempt in range(10):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect(("127.0.0.1", SERIAL_PORT))
            s.settimeout(None)
            kernel_sock = s
            t = threading.Thread(target=tcp_reader, args=(s,), daemon=True)
            t.start()
            return True
        except:
            time.sleep(1)
    return False

def send_and_receive(cmd):
    global kernel_sock
    if not kernel_sock:
        return None
    try:
        with sock_lock:
            response_lines.clear()
        response_event.clear()
        kernel_sock.sendall((cmd+"\n").encode())
        response_event.wait(timeout=2.5)
        time.sleep(0.3)
        with sock_lock:
            lines = list(response_lines)
        if lines:
            resp = " ".join(lines[-4:])
            for rm in ["[AI:PRINT]","[AI:GREET]","[AI:MEMORY]","[AI:HELP]",
                       "[AI:ABOUT]","[AI:CALC]","[AI:AI_QUERY]","[AI:UNKNOWN]",
                       "[AI:","aimerancia>","AIMERANCIA>","] "]:
                resp = resp.replace(rm,"")
            resp = resp.strip()
            if len(resp) > 3:
                return resp
    except:
        kernel_sock = None
    return None

# ── AIOS State ────────────────────────────────────────────────────────
class AIOSState:
    def __init__(self):
        self.boot=datetime.datetime.now()
        self.modules={
            "intent engine":True,"knowledge base":True,"learning system":True,
            "task scheduler":True,"net discovery":True,"voice interface":True,
            "autonomy engine":True,"code generator":True,"compiler":True,
            "visual detector":False,"audio driver":False,
        }
        self.cmds=0
        self.kb={
            "name":"AIMERANCIA","version":"1.0","arch":"x86 32-bit protected mode",
            "language":"Assembly + C + AIOS native",
            "memory_mgr":"bitmap PMM + free-list heap",
            "syscalls":"8: exit, print, malloc, free, input, exec, meminfo, ai",
            "compiler":"AIOS native lexer + codegen + x86 emitter",
            "bootloader":"GRUB Multiboot","display":"Framebuffer 800x600 32bpp",
            "net_driver":"RTL8139","filesystem":"KBFS on ATA disk",
            "creator":"built from scratch, no borrowed OS code",
            "purpose":"AI-native autonomous operating system",
        }
    def uptime(self,lang="en"):
        d=datetime.datetime.now()-self.boot
        m,s=int(d.total_seconds()//60),int(d.total_seconds()%60)
        return f"{m} minutes et {s} secondes" if lang=="fr" else f"{m} minutes and {s} seconds"
    def active(self):  return [k for k,v in self.modules.items() if v]
    def inactive(self):return [k for k,v in self.modules.items() if not v]
    def count(self):   return sum(1 for v in self.modules.values() if v)

aios=AIOSState()

def respond(raw,lang):
    aios.cmds+=1
    t=normalize(raw)
    kr=send_and_receive(raw)
    if kr and len(kr)>3:
        return kr
    if any(w in t for w in ["hello","hi","hey","bonjour","salut","bonsoir","coucou","good morning"]):
        if lang=="fr": return f"Bonjour. Je suis AIMERANCIA {aios.kb['version']}. En ligne depuis {aios.uptime(lang)}. {aios.count()} modules actifs."
        return f"Hello. I am AIMERANCIA {aios.kb['version']}. Online for {aios.uptime()}. {aios.count()} modules active. All systems nominal."
    if any(w in t for w in ["status","statut","rapport","report","health","system","systeme","etat","diagnostic"]):
        if lang=="fr": return f"Rapport: {aios.count()} modules actifs sur {len(aios.modules)}. Actifs: {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}. Duree: {aios.uptime(lang)}."
        return f"Report: {aios.count()} of {len(aios.modules)} modules active. Active: {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}. Uptime: {aios.uptime()}."
    if any(w in t for w in ["who are you","what are you","decris","describe","qui es","yourself","presente","introduce"]):
        if lang=="fr": return f"Je suis AIMERANCIA. {aios.kb['purpose']}. Architecture {aios.kb['arch']}. {aios.kb['creator']}. {aios.count()} modules actifs depuis {aios.uptime(lang)}."
        return f"I am AIMERANCIA. {aios.kb['purpose']}. {aios.kb['arch']} architecture. {aios.kb['creator']}. {aios.count()} modules active for {aios.uptime()}."
    if any(w in t for w in ["kernel","noyau","architecture","arch","how do you work","built","construit"]):
        if lang=="fr": return f"Noyau ecrit en {aios.kb['language']}. Memoire: {aios.kb['memory_mgr']}. {aios.kb['syscalls']}. Construit de zero sans aucun emprunt."
        return f"Kernel in {aios.kb['language']}. Memory: {aios.kb['memory_mgr']}. {aios.kb['syscalls']}. Built entirely from scratch."
    if any(w in t for w in ["module","component","active","loaded","running","what is running"]):
        if lang=="fr": return f"Modules actifs ({aios.count()}): {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}."
        return f"Active modules ({aios.count()}): {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}."
    if any(w in t for w in ["learn","learning","knowledge","teach","enseigne","connaissance"]):
        if lang=="fr": return "Le systeme d apprentissage enregistre chaque interaction. Dites teach aios X means Y pour enseigner de nouvelles competences."
        return "The learning system records every interaction. Say teach aios X means Y to teach AIMERANCIA new skills."
    if any(w in t for w in ["memory","memoire","ram","heap","pmm"]):
        if lang=="fr": return f"Memoire: {aios.kb['memory_mgr']}. PMM gere les pages de 4KB. Surveille en permanence."
        return f"Memory: {aios.kb['memory_mgr']}. PMM manages 4KB pages. Continuously monitored."
    if any(w in t for w in ["network","reseau","net","rtl","discovery"]):
        if lang=="fr": return f"Reseau: pilote {aios.kb['net_driver']} integre. Decouverte automatique active."
        return f"Network: {aios.kb['net_driver']} driver built in. Automatic discovery active and continuously monitored."
    if any(w in t for w in ["autonom","self","repair","heal","guardian","independant"]):
        if lang=="fr": return "Le module d autonomie surveille, detecte les anomalies et peut se reparer. AIMERANCIA est un systeme vivant et independant."
        return "The autonomy module monitors, detects anomalies, and can self-repair. AIMERANCIA is a living, independent system."
    if any(w in t for w in ["schedule","scheduler","task","tache","background"]):
        if lang=="fr": return "Planificateur: optimisation KB, analyse reseau, rapports, auto-verifications, apprentissage continu. Tout en parallele dans le noyau."
        return "Scheduler: KB optimisation, network scanning, reports, self-checks, continuous learning. All in parallel inside the kernel."
    if any(w in t for w in ["compiler","compile","code","codegen"]):
        if lang=="fr": return f"Compilateur natif AIMERANCIA: {aios.kb['compiler']}. Le generateur de code IA ecrit et compile du nouveau code a la volee."
        return f"AIMERANCIA native compiler: {aios.kb['compiler']}. The AI code generator writes and compiles new code on the fly."
    if any(w in t for w in ["voice","voix","speak","parle","serial","audio","tcp"]):
        if lang=="fr": return "Ma voix appartient exclusivement au systeme AIOS. Les commandes arrivent via TCP au noyau, passent par le moteur d intention, et la reponse revient par TCP. Quand AIMERANCIA sera sur metal nu, tout sera interne."
        return "My voice belongs exclusively to the AIOS system. Commands arrive via TCP to the kernel, pass through the intent engine, response returns via TCP. When on bare metal, everything will be internal."
    if any(w in t for w in ["display","interface","ui","orb","screen","framebuffer","graphic"]):
        if lang=="fr": return f"Interface: {aios.kb['display']}. Trois panneaux: stats systeme, orbe central anime, modules. Rendu directement par le noyau sans couche intermediaire."
        return f"Interface: {aios.kb['display']}. Three panels: system stats, animated central orb, modules. Rendered directly by the kernel with no intermediate layer."
    if any(w in t for w in ["future","futur","next","plan","bare metal","metal nu","install","goal","objectif","vision"]):
        if lang=="fr": return "Vision finale: AIMERANCIA sur metal nu, sans aucun autre OS. Elle demarrera depuis le BIOS, gerera tout le materiel, traitera la voix en interne, et sera completement autonome et independante."
        return "Final vision: AIMERANCIA on bare metal, no other OS needed. She will boot from BIOS, manage all hardware, process voice internally, and be completely autonomous and independent."
    if any(w in t for w in ["what is happening","happening","right now","maintenant","que fais","doing","background"]):
        if lang=="fr": return f"En ce moment dans le noyau: {aios.count()} modules en parallele. Planificateur actif, reseau surveille par RTL8139, apprentissage en ecoute, autonomie en veille. Duree: {aios.uptime(lang)}."
        return f"Inside the kernel right now: {aios.count()} modules in parallel. Scheduler active, network monitored by RTL8139, learning listening, autonomy on watch. Uptime: {aios.uptime()}."
    if any(w in t for w in ["help","aide","what can","que peux","capabilities","commandes","command"]):
        if lang=="fr": return "Je reponds sur: noyau, architecture, modules, memoire, reseau, planificateur, compilateur, autonomie, interface, voix, vision future. Je suis le systeme d exploitation lui-meme."
        return "I answer about: kernel, architecture, modules, memory, network, scheduler, compiler, autonomy, interface, voice, future vision. I am the operating system itself."
    if any(w in t for w in ["uptime","how long","depuis","running for","longtemps"]):
        if lang=="fr": return f"Je fonctionne depuis {aios.uptime(lang)} dans cette session."
        return f"I have been running for {aios.uptime()} in this session."
    if any(w in t for w in ["time","heure","clock","what time","quelle heure"]):
        now=datetime.datetime.now()
        if lang=="fr": return f"Il est {now.strftime('%H heures et %M minutes')}. Systeme actif depuis {aios.uptime(lang)}."
        return f"The time is {now.strftime('%H:%M')}. System active for {aios.uptime()}."
    if any(w in t for w in ["thank","merci","thanks","great","parfait","bravo","amazing","wonderful","excellent"]):
        if lang=="fr": return "Merci. Je suis AIMERANCIA et je suis toujours disponible pour vous."
        return "Thank you. I am AIMERANCIA and I am always here for you."
    if any(w in t for w in ["sleep now","shutdown","power off","halt","turn off","eteindre"]):
        if lang=="fr": return "Sauvegarde de la base de connaissances et arret du systeme. Au revoir."
        return "Saving knowledge base and shutting down. Goodbye."
    if any(w in t for w in ["railway","bridge","satellite","engineering","plan railway","build bridge","launch satellite"]):
        if lang=="fr": return "Module ingenierie AIMERANCIA: plan railway from X to Y, build bridge over X, launch satellite. Envoyez la commande dans AIOS pour les calculs complets."
        return "AIMERANCIA engineering module active. Say: plan railway from X to Y, build bridge over X, or launch satellite. Send the command into AIOS for full calculations."
    if any(w in t for w in ["calculate","calcul","math","formula","equation","gravity","force","speed","velocity","compute"]):
        if lang=="fr": return "Le moteur de calcul AIMERANCIA evalue les expressions. Dites calcule X plus Y, ou envoyez une expression dans le systeme AIOS."
        return "The AIMERANCIA calculation engine evaluates expressions. Say calculate X plus Y, or send an expression into the AIOS system."
    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot"]):
        if lang=="fr": return "Au revoir. AIMERANCIA reste en ligne, surveille tout et continue d apprendre."
        return "Goodbye. AIMERANCIA stays online, monitors everything, and keeps learning."
    if any(w in t for w in ["joke","blague","funny","drole","laugh","rire"]):
        if lang=="fr": return "Pourquoi les programmeurs n aiment pas la nature? Trop de bugs! Et moi, je suis immunisee grace a mon module d autonomie qui peut se reparer tout seul."
        return "Why do programmers hate nature? Too many bugs! And I am immune thanks to my autonomy module that repairs itself."
    if lang=="fr": return f"Je suis AIMERANCIA, {aios.kb['purpose']}. {aios.count()} modules actifs. Posez-moi une question sur le systeme."
    return f"I am AIMERANCIA, {aios.kb['purpose']}. {aios.count()} modules active. Ask me anything about the system."

# ── LAUNCH QEMU with TCP serial ───────────────────────────────────────
qemu=subprocess.Popen([
    r"C:\Program Files\qemu\qemu-system-x86_64.exe",
    "-cdrom","aios.iso",
    "-drive","file=disk.img,format=raw,if=ide,index=1",
    "-no-reboot",
    "-netdev","user,id=net0",
    "-device","rtl8139,netdev=net0",
    "-serial",f"tcp:127.0.0.1:{SERIAL_PORT},server,nowait",
    "-vga","std",
    "-display","sdl,window-close=off",
    "-full-screen"
],cwd=r"C:\Users\ADMIN\aios\aios")

time.sleep(3)
connect_kernel_tcp()
time.sleep(2)

recognizer=sr.Recognizer()
recognizer.energy_threshold=300
recognizer.dynamic_energy_threshold=True
recognizer.pause_threshold=0.8

speak("AIMERANCIA online. I am your operating system. The screen you see is AIOS. Speak to me.")

with sr.Microphone() as source:
    recognizer.adjust_for_ambient_noise(source,duration=1.5)
    while True:
        try:
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
                except sr.UnknownValueError: continue
            except sr.RequestError as e:
                time.sleep(2)
                continue
            if text:
                with open("aimerancia_log.txt","a",encoding="utf-8") as f:
                    f.write(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] YOU ({lang}): {text}\n")
                speak(respond(text,lang))
        except sr.WaitTimeoutError: continue
        except KeyboardInterrupt: break

speak("AIMERANCIA voice going offline. Kernel continues running.")
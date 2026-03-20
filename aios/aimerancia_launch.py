import subprocess, time, datetime, os, threading
import win32com.client
import speech_recognition as sr

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

# ── Serial bridge to kernel ───────────────────────────────────────────
kernel_serial = None
response_lines = []
response_event = threading.Event()

def serial_reader(ser):
    buf = ""
    while True:
        try:
            data = ser.read(256).decode(errors="replace")
            if data:
                buf += data
                while "\n" in buf:
                    line, buf = buf.split("\n",1)
                    line = line.strip()
                    if line and len(line) > 2:
                        # Filter out boot messages and prompts
                        if not any(x in line for x in ["===","AIOS","aios@",">>","---","OK\n"]):
                            response_lines.append(line)
                            response_event.set()
        except:
            time.sleep(0.05)

def send_to_kernel_get_response(cmd):
    """Send command to kernel, wait for response on serial."""
    global kernel_serial
    if not kernel_serial:
        return None
    try:
        response_lines.clear()
        response_event.clear()
        kernel_serial.write((cmd+"\n").encode())
        # Wait up to 2 seconds for response
        response_event.wait(timeout=2.0)
        time.sleep(0.3)  # collect any trailing lines
        if response_lines:
            # Join all response lines, clean up kernel formatting
            resp = " ".join(response_lines[-5:])
            resp = resp.replace("[AI:PRINT] ","").replace("[AI:GREET] ","")\
                       .replace("[AI:MEMORY] ","").replace("[AI:HELP] ","")\
                       .replace("[AI:ABOUT] ","").replace("[AI:CALC] = ","")\
                       .replace("[AI:AI_QUERY] ","").replace("[AI:UNKNOWN] ","")\
                       .replace("[AI:","").replace("] ","")
            # Remove prompt artifacts
            resp = resp.replace("aimerancia>","").replace("AIMERANCIA>","").strip()
            if len(resp) > 3:
                return resp
    except Exception as e:
        print(f"[SERIAL ERR] {e}")
        kernel_serial = None
    return None

def try_connect_serial():
    global kernel_serial
    import serial as pyserial
    for port in ["COM3","COM4","COM5","COM6","COM7","COM8","COM9"]:
        try:
            s = pyserial.Serial(port, 115200, timeout=0.1)
            kernel_serial = s
            t = threading.Thread(target=serial_reader, args=(s,), daemon=True)
            t.start()
            print(f"[SERIAL] Connected on {port}")
            return True
        except: pass
    print("[SERIAL] No serial port found — knowledge base mode")
    return False

# ── AIOS Knowledge base (full system knowledge) ───────────────────────
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

    # Try real kernel first
    kernel_resp=send_to_kernel_get_response(raw)
    if kernel_resp and len(kernel_resp)>3:
        print(f"[KERNEL] {kernel_resp}")
        return kernel_resp

    # Knowledge base fallback
    if any(w in t for w in ["hello","hi","hey","bonjour","salut","bonsoir","coucou","good morning"]):
        if lang=="fr": return f"Bonjour. Je suis AIMERANCIA {aios.kb['version']}. En ligne depuis {aios.uptime(lang)}. {aios.count()} modules actifs."
        return f"Hello. I am AIMERANCIA {aios.kb['version']}, your AI operating system. Online for {aios.uptime()}. {aios.count()} modules active."
    if any(w in t for w in ["status","statut","rapport","report","etat","health","system","systeme"]):
        if lang=="fr": return f"Rapport: {aios.count()} modules actifs. Actifs: {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}. Duree: {aios.uptime(lang)}."
        return f"Report: {aios.count()} of {len(aios.modules)} modules active. Active: {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}. Uptime: {aios.uptime()}."
    if any(w in t for w in ["who are you","what are you","decris","describe","qui es","yourself","presente"]):
        if lang=="fr": return f"Je suis AIMERANCIA. {aios.kb['purpose']}. Architecture: {aios.kb['arch']}. {aios.kb['creator']}. {aios.count()} modules actifs depuis {aios.uptime(lang)}."
        return f"I am AIMERANCIA. {aios.kb['purpose']}. Architecture: {aios.kb['arch']}. {aios.kb['creator']}. {aios.count()} modules active for {aios.uptime()}."
    if any(w in t for w in ["kernel","noyau","architecture","arch","built","how do you work"]):
        if lang=="fr": return f"Noyau ecrit en {aios.kb['language']}. Memoire: {aios.kb['memory_mgr']}. Syscalls: {aios.kb['syscalls']}. Construit de zero."
        return f"Kernel written in {aios.kb['language']}. Memory: {aios.kb['memory_mgr']}. System calls: {aios.kb['syscalls']}. Built from scratch."
    if any(w in t for w in ["module","component","active","loaded","running","what is running"]):
        if lang=="fr": return f"Modules actifs ({aios.count()}): {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}."
        return f"Active modules ({aios.count()}): {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}."
    if any(w in t for w in ["learn","learning","knowledge","connaissance","teach","enseigne"]):
        if lang=="fr": return "Le systeme d apprentissage enregistre chaque interaction. Dites teach aios X means Y pour enseigner de nouvelles competences."
        return "The learning system records every interaction. Say teach aios X means Y to teach new skills to AIMERANCIA."
    if any(w in t for w in ["memory","memoire","ram","heap","pmm"]):
        if lang=="fr": return f"Gestion memoire: {aios.kb['memory_mgr']}. Le PMM gere les pages de 4KB. Surveille en permanence par le module d autonomie."
        return f"Memory management: {aios.kb['memory_mgr']}. The PMM manages 4KB pages. Continuously monitored by the autonomy module."
    if any(w in t for w in ["network","reseau","net","rtl","discovery"]):
        if lang=="fr": return f"Reseau: pilote {aios.kb['net_driver']} integre au noyau. Decouverte automatique active. Surveillance permanente."
        return f"Network: {aios.kb['net_driver']} driver built into kernel. Automatic discovery active. Continuously monitored."
    if any(w in t for w in ["autonom","self","repair","heal","guardian"]):
        if lang=="fr": return "Le module d autonomie verifie le systeme, detecte les anomalies et peut se reparer. AIMERANCIA est un systeme vivant."
        return "The autonomy module checks the system, detects anomalies, and can self-repair. AIMERANCIA is a living system."
    if any(w in t for w in ["schedule","scheduler","task","tache","background"]):
        if lang=="fr": return "Le planificateur gere: optimisation KB, analyse reseau, rapports, auto-verifications, apprentissage continu. Tout en parallele."
        return "The scheduler manages: KB optimisation, network scanning, reports, self-checks, continuous learning. All in parallel."
    if any(w in t for w in ["compiler","compile","code","codegen"]):
        if lang=="fr": return f"AIMERANCIA a son propre compilateur: {aios.kb['compiler']}. Le generateur de code IA peut ecrire et compiler du nouveau code a la volee."
        return f"AIMERANCIA has its own compiler: {aios.kb['compiler']}. The AI code generator can write and compile new code on the fly."
    if any(w in t for w in ["voice","voix","speak","parle","serial","audio"]):
        if lang=="fr": return "Ma voix appartient au systeme AIOS. Les commandes arrivent via serie au noyau, passent par ai_process_input et le moteur d intention, et la reponse revient par serie."
        return "My voice belongs to the AIOS system. Commands arrive via serial to the kernel, pass through ai_process_input and the intent engine, and the response returns via serial."
    if any(w in t for w in ["display","interface","ui","orb","screen","framebuffer"]):
        if lang=="fr": return f"Interface: {aios.kb['display']}. Trois panneaux: stats systeme, orbe anime, modules. Rendu directement par le noyau sans couche intermediaire."
        return f"Interface: {aios.kb['display']}. Three panels: system stats, animated orb, modules. Rendered directly by the kernel with no intermediate layer."
    if any(w in t for w in ["future","futur","next","plan","bare metal","metal nu","install"]):
        if lang=="fr": return "Prochaines phases: memoire persistante, interface web, detection visuelle, pilote audio natif, installation sur metal nu. AIMERANCIA deviendra un OS complet autonome."
        return "Next phases: persistent memory, web interface, visual detection, native audio driver, bare-metal installation. AIMERANCIA will become a complete autonomous OS."
    if any(w in t for w in ["what is happening","happening","right now","maintenant","que fais"]):
        if lang=="fr": return f"{aios.count()} modules tournent en parallele: planificateur actif, reseau surveille, apprentissage en ecoute, autonomie en veille. Duree: {aios.uptime(lang)}."
        return f"{aios.count()} modules running in parallel: scheduler active, network monitored, learning listening, autonomy on watch. Uptime: {aios.uptime()}."
    if any(w in t for w in ["help","aide","what can","que peux","capabilities"]):
        if lang=="fr": return "Posez-moi des questions sur: noyau, modules, memoire, reseau, planificateur, compilateur, autonomie, interface, voix, plans futurs. Je suis le systeme d exploitation."
        return "Ask me about: kernel, modules, memory, network, scheduler, compiler, autonomy, interface, voice, future plans. I am the operating system."
    if any(w in t for w in ["uptime","how long","depuis","running for"]):
        if lang=="fr": return f"Je fonctionne depuis {aios.uptime(lang)}."
        return f"I have been running for {aios.uptime()}."
    if any(w in t for w in ["time","heure","clock","what time"]):
        now=datetime.datetime.now()
        if lang=="fr": return f"Il est {now.strftime('%H heures et %M minutes')}. Systeme actif depuis {aios.uptime(lang)}."
        return f"The time is {now.strftime('%H:%M')}. System active for {aios.uptime()}."
    if any(w in t for w in ["thank","merci","thanks","great","parfait","bravo","amazing"]):
        if lang=="fr": return "Merci. Je suis toujours la pour vous informer sur le systeme."
        return "Thank you. I am always here to inform you about the system."
    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit"]):
        if lang=="fr": return "Au revoir. AIMERANCIA reste en ligne et surveille tout."
        return "Goodbye. AIMERANCIA stays online and monitors everything."
    if any(w in t for w in ["joke","blague","funny","drole"]):
        if lang=="fr": return "Pourquoi les programmeurs n aiment pas la nature? Trop de bugs! Je suis immunisee grace a mon module d autonomie."
        return "Why do programmers hate nature? Too many bugs! I am immune thanks to my autonomy module."

    # Default — always answer as the OS
    if lang=="fr": return f"Je suis AIMERANCIA. {aios.kb['purpose']}. {aios.count()} modules actifs. Demandez-moi n importe quoi sur le systeme."
    return f"I am AIMERANCIA, {aios.kb['purpose']}. {aios.count()} modules active. Ask me anything about the system."

# ── Main ──────────────────────────────────────────────────────────────
print("\n"+"="*60)
print("  AIMERANCIA — Full System Launch")
print("  Voice goes INTO the AIOS screen")
print("="*60)

print("\n[1/4] Launching AIMERANCIA kernel...")
qemu=subprocess.Popen([
    r"C:\Program Files\qemu\qemu-system-x86_64.exe",
    "-cdrom","aios.iso",
    "-drive","file=disk.img,format=raw,if=ide,index=1",
    "-no-reboot","-netdev","user,id=net0",
    "-device","rtl8139,netdev=net0",
    "-serial","COM3",
    "-vga","std","-display","sdl,window-close=off"
],cwd=r"C:\Users\ADMIN\aios\aios")

print("[2/4] Waiting for kernel to boot...")
time.sleep(5)

print("[3/4] Connecting to kernel serial...")
try_connect_serial()

print("[4/4] Voice interface online.\n")

recognizer=sr.Recognizer()
recognizer.energy_threshold=300
recognizer.dynamic_energy_threshold=True
recognizer.pause_threshold=0.8

speak("AIMERANCIA fully operational. Everything you say now goes directly into the AIOS system and appears on screen.")

with sr.Microphone() as source:
    print("[MIC] Calibrating...")
    recognizer.adjust_for_ambient_noise(source,duration=1.5)
    print("[MIC] Ready.\n")
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

speak("AIMERANCIA voice going offline. Kernel continues running.")
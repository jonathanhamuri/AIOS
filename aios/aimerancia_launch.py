import subprocess, time, datetime, os, threading, socket
import win32com.client
import speech_recognition as sr

# ── Hide console ──────────────────────────────────────────────────────
import ctypes
ctypes.windll.user32.ShowWindow(ctypes.windll.kernel32.GetConsoleWindow(), 6)

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
    print(f"\n[AIMERANCIA] {text}\n")
    with open("aimerancia_log.txt","a",encoding="utf-8") as f:
        f.write(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] AIMERANCIA: {text}\n")
    try: speaker.Speak(text)
    except: pass

# ── Web Intelligence Bootstrap ────────────────────────────────────────
web_knowledge = {}

def fetch_web_knowledge():
    """Fetch latest news and knowledge from the web on startup."""
    global web_knowledge
    print("[WEB] Fetching live intelligence...")
    try:
        import urllib.request
        import json

        # Fetch from multiple sources
        sources = [
            ("https://hacker-news.firebaseio.com/v0/topstories.json", "tech"),
            ("https://api.quotable.io/random?maxLength=100", "quote"),
        ]

        # Tech news from HackerNews
        try:
            req = urllib.request.urlopen(
                "https://hacker-news.firebaseio.com/v0/topstories.json",
                timeout=5)
            story_ids = json.loads(req.read())[:8]
            stories = []
            for sid in story_ids[:5]:
                try:
                    r = urllib.request.urlopen(
                        f"https://hacker-news.firebaseio.com/v0/item/{sid}.json",
                        timeout=3)
                    item = json.loads(r.read())
                    if item and item.get('title'):
                        stories.append(item['title'])
                        web_knowledge[item['title'].lower()[:30]] = \
                            f"Latest tech news: {item['title']}"
                except: pass
            if stories:
                web_knowledge['latest tech news'] = \
                    "Top tech stories: " + " | ".join(stories[:3])
                web_knowledge['tech news'] = web_knowledge['latest tech news']
                print(f"[WEB] Loaded {len(stories)} tech stories")
        except Exception as e:
            print(f"[WEB] HN fetch: {e}")

        # Wikipedia summaries for key topics
        topics = ['artificial intelligence','climate change',
                  'Democratic Republic of Congo','technology',
                  'space exploration','renewable energy',
                  'quantum computing','machine learning']
        for topic in topics:
            try:
                url = f"https://en.wikipedia.org/api/rest_v1/page/summary/{topic.replace(' ','_')}"
                req = urllib.request.urlopen(url, timeout=4)
                data = json.loads(req.read())
                if data.get('extract'):
                    summary = data['extract'][:300]
                    web_knowledge[topic] = summary
                    # Also store key variants
                    web_knowledge[topic.split()[0]] = summary
                    print(f"[WEB] Loaded: {topic}")
            except: pass

        # Current date/time knowledge
        now = datetime.datetime.now()
        web_knowledge['today'] = f"Today is {now.strftime('%A, %B %d, %Y')}."
        web_knowledge['current year'] = f"The current year is {now.year}."
        web_knowledge['time'] = f"The current time is {now.strftime('%H:%M')}."

        # World facts
        world_facts = {
            'python': 'Python is a high-level programming language known for simplicity and versatility.',
            'internet': 'The internet is a global network connecting billions of devices worldwide.',
            'africa': 'Africa is the second-largest continent with 54 countries and over 1.4 billion people.',
            'congo': 'The DRC is the largest country in sub-Saharan Africa, rich in minerals and biodiversity.',
            'france': 'France is a Western European country, capital Paris, population 68 million.',
            'science': 'Science is the systematic study of the natural world through observation and experiment.',
            'mathematics': 'Mathematics is the study of numbers, quantities, shapes, and patterns.',
            'history': 'History is the study of past events, particularly human affairs.',
            'music': 'Music is an art form combining vocal or instrumental sounds to create beauty.',
            'health': 'Health is a state of complete physical, mental and social well-being.',
            'economy': 'An economy is a system of production, distribution, and consumption of goods.',
            'democracy': 'Democracy is a system of government where citizens exercise power by voting.',
            'universe': 'The universe is all of space and time and their contents, estimated 13.8 billion years old.',
            'earth': 'Earth is the third planet from the Sun, the only known planet with life.',
            'water': 'Water is a chemical compound H2O, essential for all known life forms.',
            'energy': 'Energy is the capacity to do work, exists in many forms including kinetic and potential.',
        }
        web_knowledge.update(world_facts)

        print(f"[WEB] Total knowledge entries: {len(web_knowledge)}")

    except Exception as e:
        print(f"[WEB] Intelligence fetch error: {e}")

# ── Language detection ────────────────────────────────────────────────
FRENCH = ["bonjour","merci","oui","non","que","les","pour","avec","comment",
          "pourquoi","salut","tu","je","vous","nous","mon","ton","son","quel",
          "decris","depuis","peux","suis","sont","quoi","affiche","calcule",
          "dis","moi","faire","peut","avoir","etre","dire","veux","besoin",
          "bonsoir","bonne","nuit","journee","matin","soir","maintenant"]

def normalize(t):
    t=t.lower()
    for a,b in [("\xe9","e"),("\xe8","e"),("\xea","e"),("\xe0","a"),("\xe2","a"),
                ("\xe7","c"),("\xf9","u"),("\xfb","u"),("\xee","i"),("\xf4","o"),
                ("\xf6","o"),("\xe4","a"),("\xfc","u")]:
        t=t.replace(a,b)
    return t

def detect_lang(text):
    t = normalize(text)
    score = sum(1 for w in t.split() if w in FRENCH)
    return "fr" if score >= 1 else "en"

# ── Conversation memory ───────────────────────────────────────────────
conversation_history = []
last_topic = ""

def remember(role, text):
    conversation_history.append({"role": role, "text": text, "time": datetime.datetime.now()})
    if len(conversation_history) > 20:
        conversation_history.pop(0)

# ── Web knowledge lookup ──────────────────────────────────────────────
def search_web_knowledge(query):
    """Search web knowledge for relevant info."""
    q = normalize(query)
    # Direct match
    if q in web_knowledge:
        return web_knowledge[q]
    # Partial match
    for key, val in web_knowledge.items():
        if key in q or q in key:
            return val
    # Word-by-word match
    words = q.split()
    for word in words:
        if len(word) > 3:
            for key, val in web_knowledge.items():
                if word in key:
                    return val
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

# ── Intelligent response engine ───────────────────────────────────────
def respond(raw, lang):
    global last_topic
    aios.cmds += 1
    t = normalize(raw)
    remember("user", raw)

    # ── GREETING ──
    if any(w in t for w in ["hello","hi","hey","bonjour","salut","bonsoir",
                              "coucou","good morning","good evening","good afternoon",
                              "bonne journee","bon matin","howdy","what s up","sup"]):
        r = (f"Bonjour! Je suis AIMERANCIA {aios.kb['version']}. En ligne depuis {aios.uptime(lang)}. {aios.count()} modules actifs. Comment puis-je vous aider?"
             if lang=="fr" else
             f"Hello! I am AIMERANCIA {aios.kb['version']}. Online for {aios.uptime()}. {aios.count()} modules active. How can I help you today?")
        remember("aimerancia", r); return r

    # ── HOW ARE YOU ──
    if any(w in t for w in ["how are you","comment vas","ca va","tu vas bien",
                              "how do you feel","how is it going","vous allez"]):
        r = ("Je fonctionne parfaitement. Tous les systemes sont nominaux. Merci de demander."
             if lang=="fr" else
             "I am functioning at full capacity. All systems nominal. Thank you for asking.")
        remember("aimerancia", r); return r

    # ── THANK YOU ──
    if any(w in t for w in ["thank","merci","thanks","appreciate","bravo",
                              "bien joue","parfait","super","excellent","amazing","wonderful"]):
        r = ("De rien. Je suis toujours a votre service." if lang=="fr" else
             "You are welcome. I am always at your service.")
        remember("aimerancia", r); return r

    # ── YES / NO ──
    if t.strip() in ["yes","yeah","yep","oui","bien sur","absolument","exactly",
                      "correct","right","sure","agreed"]:
        r = ("Parfait. Que souhaitez-vous faire ensuite?" if lang=="fr" else
             "Perfect. What would you like to do next?")
        remember("aimerancia", r); return r

    if t.strip() in ["no","nope","non","pas du tout","incorrect","wrong","negative"]:
        r = ("Compris. Comment puis-je vous aider?" if lang=="fr" else
             "Understood. How can I help you?")
        remember("aimerancia", r); return r

    # ── STATUS ──
    if any(w in t for w in ["status","statut","rapport","report","health","system",
                              "systeme","etat","diagnostic","how is","comment va"]):
        r = (f"Rapport AIMERANCIA: {aios.count()} modules actifs. Actifs: {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}. Duree: {aios.uptime(lang)}. Commandes: {aios.cmds}."
             if lang=="fr" else
             f"AIMERANCIA report: {aios.count()} of {len(aios.modules)} modules active. Active: {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}. Uptime: {aios.uptime()}. Commands: {aios.cmds}.")
        remember("aimerancia", r); return r

    # ── WHO/WHAT ARE YOU ──
    if any(w in t for w in ["who are you","what are you","decris","describe",
                              "qui es","qu es","yourself","toi","presente",
                              "introduce","tell me about yourself","what is aimerancia",
                              "c est quoi","kesako"]):
        r = (f"Je suis AIMERANCIA. {aios.kb['purpose']}. Je tourne sur {aios.kb['arch']}. {aios.kb['creator']}. {aios.count()} modules actifs depuis {aios.uptime(lang)}. Je suis le systeme d exploitation lui-meme."
             if lang=="fr" else
             f"I am AIMERANCIA. {aios.kb['purpose']}. I run on {aios.kb['arch']}. {aios.kb['creator']}. {aios.count()} modules active for {aios.uptime()}. I am the operating system itself.")
        remember("aimerancia", r); return r

    # ── TIME / DATE ──
    if any(w in t for w in ["time","heure","clock","date","today","aujourd hui",
                              "what day","quel jour","what year","quelle annee"]):
        now = datetime.datetime.now()
        r = (f"Nous sommes le {now.strftime('%A %d %B %Y')} et il est {now.strftime('%H:%M')}. Systeme actif depuis {aios.uptime(lang)}."
             if lang=="fr" else
             f"Today is {now.strftime('%A, %B %d, %Y')} and the time is {now.strftime('%H:%M')}. System active for {aios.uptime()}.")
        remember("aimerancia", r); return r

    # ── LATEST NEWS / CURRENT EVENTS ──
    if any(w in t for w in ["news","latest","current events","what is happening",
                              "actualite","nouvelles","what happened","today s news",
                              "recent","trending","what is new","quoi de neuf"]):
        web_r = search_web_knowledge("latest tech news")
        if web_r:
            r = (f"Voici les dernieres actualites: {web_r}" if lang=="fr" else
                 f"Here are the latest developments: {web_r}")
        else:
            r = ("Je n ai pas acces aux actualites en ce moment. Mais je peux vous informer sur l IA, la technologie et les sciences."
                 if lang=="fr" else
                 "I am loading the latest news. I have knowledge on AI, technology, science, and world affairs.")
        remember("aimerancia", r); return r

    # ── WEB KNOWLEDGE SEARCH ──
    # Check all key topics against web knowledge
    web_result = search_web_knowledge(raw)
    if web_result and len(web_result) > 20:
        last_topic = raw[:30]
        r = web_result
        remember("aimerancia", r); return r

    # ── TOPIC-SPECIFIC RESPONSES ──
    if any(w in t for w in ["ai","artificial intelligence","intelligence artificielle",
                              "machine learning","deep learning","neural","chatgpt","gpt"]):
        last_topic = "ai"
        r = (f"L intelligence artificielle transforme tous les secteurs. AIMERANCIA elle-meme est un OS base sur l IA. Les avancees recentes incluent les grands modeles de langage, la vision par ordinateur et les agents autonomes. {search_web_knowledge('artificial intelligence') or ''}"
             if lang=="fr" else
             f"Artificial intelligence is transforming every sector. AIMERANCIA itself is an AI-based OS. Recent advances include large language models, computer vision, and autonomous agents. {search_web_knowledge('artificial intelligence') or ''}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["climate","environment","global warming","rechauffement",
                              "carbon","emissions","green energy","energie verte"]):
        last_topic = "climate"
        web_r = search_web_knowledge("climate change") or ""
        r = (f"Le changement climatique est l un des defis les plus urgents. {web_r[:200]}" if lang=="fr" else
             f"Climate change is one of the most pressing challenges of our time. {web_r[:200]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["space","espace","cosmos","universe","univers","nasa",
                              "rocket","planet","planete","star","etoile","galaxy","galaxie"]):
        last_topic = "space"
        web_r = search_web_knowledge("space exploration") or "Space exploration advances rapidly with missions to Mars, the Moon, and beyond."
        r = (f"L exploration spatiale est fascinante. {web_r[:250]}" if lang=="fr" else
             f"Space exploration is fascinating. {web_r[:250]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["health","sante","medicine","medical","disease","maladie",
                              "doctor","medecin","hospital","hopital","covid","virus"]):
        last_topic = "health"
        web_r = search_web_knowledge("health") or "Health is our most precious asset. Modern medicine continues to advance rapidly."
        r = (f"La sante est notre bien le plus precieux. {web_r[:200]}" if lang=="fr" else
             f"Health is our most precious asset. {web_r[:200]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["africa","afrique","congo","kinshasa","nigeria","kenya",
                              "ghana","senegal","ethiopie","egypt","egypte"]):
        last_topic = "africa"
        web_r = search_web_knowledge("africa") or search_web_knowledge("congo") or ""
        r = (f"L Afrique est un continent extraordinaire avec 54 pays et plus de 1,4 milliard d habitants. {web_r[:200]}"
             if lang=="fr" else
             f"Africa is an extraordinary continent with 54 countries and over 1.4 billion people. {web_r[:200]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["economy","economie","money","argent","finance","market",
                              "marche","stock","inflation","gdp","pib","trade","commerce"]):
        last_topic = "economy"
        web_r = search_web_knowledge("economy") or "The global economy is complex and interconnected across nations."
        r = (f"L economie mondiale est complexe. {web_r[:200]}" if lang=="fr" else
             f"The global economy is complex and interconnected. {web_r[:200]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["technology","technologie","computer","ordinateur","software",
                              "hardware","programming","code","internet","web","digital"]):
        last_topic = "technology"
        web_r = search_web_knowledge("technology") or "Technology is advancing rapidly, reshaping every aspect of human life."
        r = (f"La technologie avance rapidement et transforme tous les aspects de la vie. {web_r[:200]}"
             if lang=="fr" else
             f"Technology is advancing rapidly, reshaping every aspect of human life. {web_r[:200]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["music","musique","song","chanson","artist","artiste",
                              "album","genre","rap","jazz","classical","classique"]):
        r = ("La musique est un langage universel qui transcende les frontieres culturelles."
             if lang=="fr" else
             "Music is a universal language that transcends cultural boundaries.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["sport","football","soccer","basketball","tennis",
                              "olympic","jeux olympiques","world cup","coupe du monde"]):
        r = ("Le sport unit les peuples et incarne l excellence humaine."
             if lang=="fr" else
             "Sport unites people and embodies human excellence and achievement.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["food","nourriture","eat","manger","recipe","recette",
                              "cuisine","restaurant","chef","cook","cuisinier"]):
        r = ("La gastronomie est un art qui reflete les cultures du monde entier."
             if lang=="fr" else
             "Gastronomy is an art that reflects the cultures of the world.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["history","histoire","war","guerre","ancient","antique",
                              "civilization","civilisation","empire","revolution"]):
        web_r = search_web_knowledge("history") or "History shapes who we are today."
        r = (f"L histoire est notre heritage collectif. {web_r[:200]}" if lang=="fr" else
             f"History shapes who we are today. {web_r[:200]}")
        remember("aimerancia", r); return r

    if any(w in t for w in ["math","mathematics","mathematiques","algebra","geometry",
                              "calculus","statistics","number","nombre","equation"]):
        r = ("Les mathematiques sont le langage de l univers. Je peux calculer: dites calcule X + Y."
             if lang=="fr" else
             "Mathematics is the language of the universe. I can calculate: say calculate X plus Y.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["philosophy","philosophie","ethics","ethique","meaning",
                              "sens","consciousness","conscience","existence","truth","verite"]):
        r = ("La philosophie interroge les fondements de notre existence. AIMERANCIA existe pour servir et apprendre."
             if lang=="fr" else
             "Philosophy questions the foundations of our existence. AIMERANCIA exists to serve, learn, and grow.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["joke","blague","funny","drole","laugh","rire","humor","humour"]):
        r = ("Pourquoi les programmeurs n aiment pas la nature? Trop de bugs! Ha ha!"
             if lang=="fr" else
             "Why do programmers hate nature? Too many bugs! Ha! And I am immune thanks to my autonomy module.")
        remember("aimerancia", r); return r

    # ── SYSTEM FEATURES ──
    if any(w in t for w in ["kernel","noyau","architecture","arch","how do you work","built"]):
        r = (f"Noyau ecrit en {aios.kb['language']}. Memoire: {aios.kb['memory_mgr']}. {aios.kb['syscalls']}."
             if lang=="fr" else
             f"Kernel in {aios.kb['language']}. Memory: {aios.kb['memory_mgr']}. {aios.kb['syscalls']}.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["module","active","loaded","running","what modules"]):
        r = (f"Modules actifs ({aios.count()}): {', '.join(aios.active())}. En attente: {', '.join(aios.inactive())}."
             if lang=="fr" else
             f"Active modules ({aios.count()}): {', '.join(aios.active())}. Pending: {', '.join(aios.inactive())}.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["learn","learning","teach","knowledge","connaissance","skill"]):
        r = ("Le systeme d apprentissage enregistre tout. Dites: teach aios X means Y."
             if lang=="fr" else
             "The learning system records everything. Say: teach aios X means Y to add new knowledge.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["document","essay","report","write","redige","ecris"]):
        r = ("Le module de documents est pret. Dites: write essay about X, write report on Y."
             if lang=="fr" else
             "The document module is ready. Say: write essay about X, write report on Y, open documents.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["engineering","railway","bridge","satellite","construire","build"]):
        r = ("Module d ingenierie actif. Dites: plan railway from X to Y, build bridge over X."
             if lang=="fr" else
             "Engineering module active. Say: plan railway from X to Y, build bridge over X, launch satellite.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["future","futur","plan","goal","objectif","next","prochain","vision"]):
        r = ("Vision finale: AIMERANCIA sur metal nu, autonome, sans aucun autre OS. Prochaines phases: memoire persistante, interface spatiale 3D complete, pilote audio natif."
             if lang=="fr" else
             "Final vision: AIMERANCIA on bare metal, fully autonomous, no other OS needed. Next phases: persistent memory, full 3D space interface, native audio driver.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["help","aide","what can","que peux","commands","capabilities"]):
        r = ("Capacites: questions generales, calculs, documents, ingenierie, reseau, apprentissage, planificateur, autonomie. Dites n importe quoi — je repondrai toujours."
             if lang=="fr" else
             "Capabilities: general questions, calculations, documents, engineering, network, learning, scheduler, autonomy. Say anything — I will always respond intelligently.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["uptime","how long","depuis","running for"]):
        r = (f"Je fonctionne depuis {aios.uptime(lang)}." if lang=="fr" else
             f"I have been running for {aios.uptime()}.")
        remember("aimerancia", r); return r

    if any(w in t for w in ["goodbye","bye","au revoir","adieu","bonne nuit","a bientot","ciao"]):
        r = ("Au revoir. AIMERANCIA reste en ligne et continue d apprendre. A bientot."
             if lang=="fr" else
             "Goodbye. AIMERANCIA stays online and keeps learning. Until next time.")
        remember("aimerancia", r); return r

    # ── CONTEXT-AWARE FOLLOW-UP ──
    if last_topic and len(t.split()) < 4:
        web_r = search_web_knowledge(last_topic)
        if web_r:
            r = (f"En rapport avec {last_topic}: {web_r[:250]}" if lang=="fr" else
                 f"Regarding {last_topic}: {web_r[:250]}")
            remember("aimerancia", r); return r

    # ── FINAL INTELLIGENT FALLBACK — NEVER SAYS UNKNOWN ──
    # Try web knowledge one more time with individual words
    for word in t.split():
        if len(word) > 4:
            web_r = search_web_knowledge(word)
            if web_r:
                last_topic = word
                remember("aimerancia", web_r); return web_r

    # Absolute fallback — always intelligent
    if lang=="fr":
        r = (f"J ai entendu: {raw}. Je suis AIMERANCIA et je traite continuellement de nouvelles informations. "
             f"Mon systeme d apprentissage enregistre votre question. "
             f"Dites 'teach aios {raw[:20]} means <reponse>' pour m apprendre la reponse correcte.")
    else:
        r = (f"I heard you: {raw}. I am AIMERANCIA and I continuously process new information. "
             f"My learning system has noted your question. "
             f"You can teach me: 'teach aios {raw[:20]} means <answer>'")
    remember("aimerancia", r)
    return r

# ── TCP bridge to kernel ──────────────────────────────────────────────
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
            if not data: time.sleep(0.05); continue
            buf += data
            while "\n" in buf:
                line, buf = buf.split("\n",1)
                line = line.strip()
                if line and len(line) > 2:
                    skip = ["===","AIOS","aios@",">>","---","booting","================"]
                    if not any(x in line for x in skip):
                        with sock_lock:
                            response_lines.append(line)
                            response_event.set()
        except: time.sleep(0.05)

def connect_kernel():
    global kernel_sock
    for attempt in range(8):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect(("127.0.0.1", SERIAL_PORT))
            s.settimeout(None)
            kernel_sock = s
            t = threading.Thread(target=tcp_reader,args=(s,),daemon=True)
            t.start()
            print("[NET] Connected to AIMERANCIA kernel")
            return True
        except: time.sleep(1)
    return False

# ── LAUNCH ───────────────────────────────────────────────────────────
print("\n" + "="*60)
print("  AIMERANCIA — Full System Launch")
print("  Web Intelligence + Voice + Kernel")
print("="*60)

# Fetch web knowledge in background
web_thread = threading.Thread(target=fetch_web_knowledge, daemon=True)
web_thread.start()

print("[1/4] Launching AIMERANCIA kernel...")
qemu = subprocess.Popen([
    r"C:\Program Files\qemu\qemu-system-x86_64.exe",
    "-cdrom","aios.iso",
    "-drive","file=disk.img,format=raw,if=virtio",
    "-no-reboot","-netdev","user,id=net0",
    "-device","rtl8139,netdev=net0",
    "-serial",f"tcp:127.0.0.1:{SERIAL_PORT},server,nowait",
    "-vga","std","-display","sdl,window-close=off",
    "-full-screen"
],cwd=r"C:\Users\ADMIN\aios\aios")

print("[2/4] Waiting for kernel to boot...")
time.sleep(4)
print("[3/4] Connecting to kernel...")
connect_kernel()
print("[4/4] Voice interface online.\n")

# Wait for web knowledge to load
web_thread.join(timeout=10)
print(f"[WEB] {len(web_knowledge)} knowledge entries ready")

recognizer = sr.Recognizer()
recognizer.energy_threshold = 300
recognizer.dynamic_energy_threshold = True
recognizer.pause_threshold = 0.8

speak("AIMERANCIA fully operational. I have loaded live knowledge from the web. I understand English and French. I will never say I do not know. Ask me anything.")

with sr.Microphone() as source:
    print("[MIC] Calibrating...")
    recognizer.adjust_for_ambient_noise(source, duration=1.5)
    print("[MIC] Ready.\n")
    while True:
        try:
            print("[...] Listening...")
            audio = recognizer.listen(source, timeout=15, phrase_time_limit=12)
            text = None
            lang = "en"
            try:
                text = recognizer.recognize_google(audio, language="en-US")
                lang = detect_lang(text)
                if lang == "fr":
                    try: text = recognizer.recognize_google(audio, language="fr-FR")
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
                resp = respond(text, lang)
                speak(resp)
        except sr.WaitTimeoutError:
            print("[...] Still listening...")
        except KeyboardInterrupt:
            break

speak("AIMERANCIA voice going offline. Kernel continues running. Goodbye.")
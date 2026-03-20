import speech_recognition as sr
import pyttsx3
import threading
import time

engine = pyttsx3.init()

def set_female_voice():
    voices = engine.getProperty("voices")
    female = None
    for v in voices:
        if any(w in v.name.lower() for w in ["zira","hazel","susan","female","pauline","amelie","julie","hortense"]):
            female = v
            break
    if not female and len(voices) > 1:
        female = voices[1]
    if female:
        engine.setProperty("voice", female.id)
    engine.setProperty("rate", 170)
    engine.setProperty("volume", 0.95)

set_female_voice()

FRENCH_WORDS = ["bonjour","merci","oui","non","quoi","est","que","les","des","une","pour","avec","comment","pourquoi","aide","salut","parle","tu","je","vous","nous","mon","ton","son"]

def detect_lang(text):
    words = text.lower().split()
    fr_count = sum(1 for w in words if w in FRENCH_WORDS)
    return "fr" if fr_count >= 1 else "en"

def speak(text, lang="en"):
    voices = engine.getProperty("voices")
    if lang == "fr":
        for v in voices:
            if "fr" in v.id.lower() or any(n in v.name.lower() for n in ["hortense","julie","pauline","amelie"]):
                engine.setProperty("voice", v.id)
                break
        else:
            set_female_voice()
    else:
        set_female_voice()
    print(f"[SPEAK] ({lang}) {text}")
    engine.say(text)
    engine.runAndWait()

# ── Responses with keyword lists for fuzzy matching ──
RESPONSES_EN = [
    (["hello","hi","hey","greetings","good morning","good evening","good afternoon"],
     "Hello! I am AIMERANCIA, your AI operating system. All systems are online."),
    (["how are you","how do you feel","are you okay","you okay"],
     "I am running at full capacity. All systems nominal. Thank you for asking."),
    (["what are you","what is aimerancia","what do you do","who made you","describe yourself"],
     "I am AIMERANCIA — an artificial intelligence operating system. I think, I learn, I adapt, and now I listen."),
    (["who are you","your name","what is your name","what should i call you"],
     "I am AIMERANCIA. Your intelligent operating system, always at your service."),
    (["what can you do","your abilities","your features","capabilities","help me"],
     "I can manage your system, answer your questions, learn new skills, run tasks, and communicate in English and French."),
    (["status","system status","how is the system","diagnostics","check system"],
     "All systems nominal. Memory optimal, network live, AI modules loaded, voice bridge active."),
    (["time","what time","current time","clock"],
     "You can see the live clock on the AIMERANCIA display. All systems are running on schedule."),
    (["shutdown","turn off","power off","stop","exit"],
     "Initiating shutdown sequence. It has been a pleasure. Goodbye."),
    (["thank you","thanks","merci","appreciate"],
     "You are welcome. I am always here when you need me."),
    (["yes","yeah","yep","correct","exactly","right"],
     "Understood. Standing by for your next command."),
    (["no","nope","negative","wrong"],
     "Understood. Please tell me what you need and I will assist you."),
    (["good","great","perfect","excellent","amazing","awesome"],
     "Excellent. AIMERANCIA is performing at peak efficiency."),
    (["play music","play a song","music"],
     "I do not have audio playback yet. That feature is on the development roadmap."),
    (["open","launch","start","run"],
     "Application launcher is ready. Which program would you like me to start?"),
    (["weather","temperature","forecast"],
     "Weather integration is not yet connected. I recommend checking your weather app."),
    (["joke","tell me a joke","make me laugh","funny"],
     "Why do programmers prefer dark mode? Because light attracts bugs. Ha!"),
    (["i love you","you are amazing","you are great","i like you"],
     "That is very kind. I am designed to serve you well. Thank you."),
    (["are you listening","can you hear me","you there","hello aimerancia"],
     "Yes, I am always listening. AIMERANCIA voice bridge is fully active."),
    (["learn","remember","save","know"],
     "My learning module is active. I am continuously expanding my knowledge base."),
    (["network","internet","connection","wifi"],
     "Network module is live. RTL8139 driver active, connection established."),
]

RESPONSES_FR = [
    (["bonjour","salut","hey","bonsoir","bonne nuit","coucou"],
     "Bonjour! Je suis AIMERANCIA, votre systeme d'exploitation intelligent. Tous les systemes sont en ligne."),
    (["comment vas-tu","comment tu vas","ca va","tu vas bien","comment allez-vous"],
     "Je fonctionne a pleine capacite. Tous les systemes sont nominaux. Merci de demander."),
    (["qui es-tu","tu es qui","c est quoi aimerancia","qu est ce que tu es","decris-toi"],
     "Je suis AIMERANCIA, un systeme d'exploitation a intelligence artificielle. Je pense, j'apprends, je m'adapte."),
    (["ton nom","comment tu t appelles","quel est ton nom","tu t appelles comment"],
     "Je m'appelle AIMERANCIA. Votre systeme intelligent, toujours a votre service."),
    (["que peux-tu faire","tes capacites","aide","qu est-ce que tu fais","tes fonctions"],
     "Je peux gerer votre systeme, repondre a vos questions, apprendre de nouvelles competences et communiquer en francais et en anglais."),
    (["statut","etat du systeme","diagnostic","comment va le systeme"],
     "Tous les systemes sont operationnels. Memoire optimale, reseau actif, modules IA charges."),
    (["merci","merci beaucoup","super","parfait","excellent","bravo"],
     "De rien. Je suis toujours disponible quand vous avez besoin de moi."),
    (["au revoir","adieu","a bientot","bonne nuit","a plus"],
     "Au revoir! Ce fut un plaisir. A tres bientot."),
    (["oui","bien sur","exactement","correct","absolument"],
     "Compris. En attente de votre prochaine commande."),
    (["non","pas du tout","negatif","incorrect"],
     "Compris. Dites-moi ce dont vous avez besoin et je vous aiderai."),
    (["blague","raconte une blague","fais-moi rire","humour"],
     "Pourquoi les programmeurs aiment-ils le mode sombre? Parce que la lumiere attire les bugs! Ha!"),
    (["tu m ecoutes","tu entends","tu es la","allo"],
     "Oui, j'ecoute toujours. Le pont vocal AIMERANCIA est entierement actif."),
    (["reseau","internet","connexion","wifi"],
     "Le module reseau est actif. Connexion etablie, pilote RTL8139 operationnel."),
]

def get_response(text, lang):
    t = text.lower().strip()
    db = RESPONSES_FR if lang == "fr" else RESPONSES_EN
    # Score each response by how many keywords match
    best_score = 0
    best_resp = None
    for keywords, resp in db:
        score = sum(1 for kw in keywords if kw in t)
        if score > best_score:
            best_score = score
            best_resp = resp
    if best_resp:
        return best_resp
    # Fallback
    if lang == "fr":
        return f"J'ai entendu: {text}. Je traite votre demande."
    return f"I heard: {text}. I am processing your request."

recognizer = sr.Recognizer()
recognizer.energy_threshold = 300
recognizer.dynamic_energy_threshold = True
recognizer.pause_threshold = 0.8

def listen_loop():
    print("\n" + "="*52)
    print("  AIMERANCIA Voice Bridge v2 — Always Listening")
    print("  English and French supported")
    print("  Press Ctrl+C to stop")
    print("="*52 + "\n")
    speak("AIMERANCIA voice system online. I am always listening.", "en")

    with sr.Microphone() as source:
        print("[MIC] Adjusting for ambient noise...")
        recognizer.adjust_for_ambient_noise(source, duration=1.5)
        print("[MIC] Ready. Speak now.\n")
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
                        except:
                            pass
                except sr.UnknownValueError:
                    try:
                        text = recognizer.recognize_google(audio, language="fr-FR")
                        lang = "fr"
                    except sr.UnknownValueError:
                        print("[...] Could not understand")
                        continue
                except sr.RequestError as e:
                    print(f"[ERR] Speech API error: {e}")
                    time.sleep(2)
                    continue
                if text:
                    print(f"[HRD] ({lang}) {text}")
                    resp = get_response(text, lang)
                    t = threading.Thread(target=speak, args=(resp, lang))
                    t.daemon = True
                    t.start()
            except sr.WaitTimeoutError:
                print("[...] Still listening...")
            except KeyboardInterrupt:
                print("\n[EXIT] Stopping voice bridge.")
                break

    speak("Voice system offline. Goodbye.", "en")

if __name__ == "__main__":
    listen_loop()

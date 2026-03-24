#include "doc_page.h"
#include "../../terminal/terminal.h"

/* Long document generator — essays, reports, assignments up to 40 pages */
/* Each "page" = ~400 words = ~2400 chars */

static void dcat_safe(char*d, const char*s, int m){
    int i=0; while(d[i])i++;
    while(*s && i<m-2){d[i++]=*s++;} d[i]=0;
}

/* ── Section builders ── */
static void add_section(char*b, int m, const char*title, const char*body){
    dcat_safe(b,"\n",m);
    dcat_safe(b,title,m);
    dcat_safe(b,"\n\n",m);
    dcat_safe(b,body,m);
    dcat_safe(b,"\n\n",m);
}

static void add_subsection(char*b, int m, const char*title, const char*body){
    dcat_safe(b,title,m);
    dcat_safe(b,"\n\n",m);
    dcat_safe(b,body,m);
    dcat_safe(b,"\n\n",m);
}

static void add_citation(char*b, int m, int num, const char*author,
                          const char*year, const char*title_ref,
                          const char*journal){
    char line[256]={0};
    /* Format: [N] Author, A. (Year). Title. Journal. */
    line[0]='['; line[1]='0'+num/10; line[2]='0'+num%10;
    if(num<10){line[1]='0'+num; line[2]=']'; line[3]=' '; line[4]=0;}
    else{line[3]=']'; line[4]=' '; line[5]=0;}
    dcat_safe(b,line,m);
    dcat_safe(b,author,m); dcat_safe(b," (",m);
    dcat_safe(b,year,m);   dcat_safe(b,"). ",m);
    dcat_safe(b,title_ref,m); dcat_safe(b,". ",m);
    dcat_safe(b,journal,m); dcat_safe(b,".\n",m);
}

/* ── Topic-aware content database ── */
typedef struct {
    const char* keyword;
    const char* intro;
    const char* background;
    const char* analysis1_title;
    const char* analysis1;
    const char* analysis2_title;
    const char* analysis2;
    const char* analysis3_title;
    const char* analysis3;
    const char* implications;
    const char* conclusion;
    const char* ref1_author; const char* ref1_year; const char* ref1_title; const char* ref1_journal;
    const char* ref2_author; const char* ref2_year; const char* ref2_title; const char* ref2_journal;
    const char* ref3_author; const char* ref3_year; const char* ref3_title; const char* ref3_journal;
    const char* ref4_author; const char* ref4_year; const char* ref4_title; const char* ref4_journal;
} topic_db_t;

static topic_db_t topics[] = {
  { "ai",
    "Artificial intelligence (AI) represents one of the most profound technological transformations in human history. Defined broadly as the simulation of human intelligence processes by computer systems, AI encompasses machine learning, natural language processing, computer vision, and autonomous decision-making. The rapid advancement of AI capabilities over the past decade has generated both extraordinary opportunities and significant challenges across virtually every domain of human activity. This document examines the multifaceted dimensions of artificial intelligence, exploring its theoretical foundations, practical applications, societal implications, and future trajectories [1].",
    "The conceptual foundations of AI trace back to Alan Turing's seminal 1950 paper 'Computing Machinery and Intelligence,' which posed the now-famous question: 'Can machines think?' [2]. The formal discipline emerged at the 1956 Dartmouth Conference, where pioneers including John McCarthy, Marvin Minsky, and Claude Shannon laid the groundwork for AI as a scientific field. Early optimism gave way to periods of reduced funding known as 'AI winters,' before the convergence of big data, increased computational power, and algorithmic innovations in deep learning sparked a renaissance beginning around 2010. Today, AI systems achieve superhuman performance in specific domains including chess, Go, protein structure prediction, and medical image diagnosis [3].",
    "Machine Learning and Deep Learning",
    "Machine learning constitutes the dominant paradigm in contemporary AI development. Rather than explicit programming, ML systems learn patterns from data through exposure to examples. Supervised learning trains models on labeled datasets, enabling tasks such as image classification and speech recognition. Unsupervised learning discovers hidden structure in unlabeled data through clustering and dimensionality reduction. Reinforcement learning enables agents to learn optimal behaviors through trial-and-error interaction with environments, achieving remarkable results in game playing and robotic control [4]. Deep learning, employing artificial neural networks with multiple layers, has proven particularly powerful, enabling breakthroughs in natural language understanding, image generation, and scientific discovery. The transformer architecture introduced in 2017 revolutionized NLP, enabling large language models capable of sophisticated reasoning and generation.",
    "AI Applications Across Sectors",
    "The deployment of AI technologies spans virtually every economic sector. In healthcare, AI algorithms analyze medical images with diagnostic accuracy matching or exceeding specialist physicians, accelerating drug discovery, personalizing treatment protocols, and predicting patient deterioration. The AlphaFold system developed by DeepMind solved the 50-year-old protein folding problem, potentially transforming drug development [1]. In finance, AI powers algorithmic trading, fraud detection, credit risk assessment, and customer service automation. The financial sector's AI adoption is estimated to generate 1 trillion dollars in annual value by 2030. Transportation is being transformed by autonomous vehicle technology, with systems capable of navigating complex urban environments through sensor fusion and real-time decision-making. Manufacturing benefits from predictive maintenance, quality control, and supply chain optimization through AI-driven analytics. Education is experiencing personalization through adaptive learning platforms that adjust content difficulty and presentation to individual student needs and learning styles.",
    "Ethical and Societal Dimensions",
    "The rapid deployment of AI raises profound ethical questions that societies must urgently address. Algorithmic bias represents a critical concern, as AI systems trained on historical data may perpetuate or amplify existing social inequalities. Studies have demonstrated that facial recognition systems exhibit significantly higher error rates for darker-skinned individuals and women [3]. Privacy implications of AI-powered surveillance, data collection, and behavioral prediction require robust regulatory frameworks. The question of algorithmic transparency and explainability becomes critical when AI systems inform consequential decisions in criminal justice, credit allocation, and medical treatment. Labor market disruption from automation threatens displacement of routine cognitive and physical work, requiring proactive policies for workforce transition and education. The concentration of AI capabilities in a small number of technology corporations raises concerns about economic inequality and power asymmetry.",
    "AI governance and policy frameworks have emerged as priorities for national governments and international bodies. The European Union's AI Act establishes risk-based regulation of AI systems, prohibiting certain high-risk applications and imposing transparency requirements. The United States has pursued a more sector-specific regulatory approach while investing heavily in AI research through national initiatives. China has implemented comprehensive AI development plans targeting leadership in key application areas by 2030. International cooperation on AI safety standards, data governance, and preventing arms races in autonomous weapons remains challenging given geopolitical tensions. The development of beneficial AI that remains aligned with human values represents the central technical and governance challenge of the field.",
    "This analysis demonstrates that artificial intelligence is not merely a technological development but a civilizational transformation requiring coordinated responses across technical, policy, ethical, and educational dimensions. The decisions made in the next decade regarding AI development, deployment, and governance will shape human society for generations. Realizing the potential of AI to address humanity's greatest challenges while mitigating risks requires sustained interdisciplinary collaboration, inclusive governance frameworks, and commitment to human-centered values [2].",
    "Russell, S. & Norvig, P.","2021","Artificial Intelligence: A Modern Approach","Pearson Education",
    "Turing, A.M.","1950","Computing Machinery and Intelligence","Mind Journal vol 59",
    "Buolamwini, J. & Gebru, T.","2018","Gender Shades: Intersectional Accuracy Disparities in Commercial Gender Classification","Conference on Fairness Accountability and Transparency",
    "LeCun, Y. Bengio, Y. & Hinton, G.","2015","Deep Learning","Nature vol 521 pp 436-444"
  },
  { "climate",
    "Climate change represents the defining environmental challenge of the twenty-first century, threatening the ecological foundations upon which human civilization depends. The scientific consensus, based on decades of research from thousands of scientists across disciplines, unequivocally establishes that human activities are causing unprecedented warming of the Earth's climate system [1]. Global average temperatures have already increased approximately 1.1 degrees Celsius above pre-industrial levels, with consequences including rising sea levels, intensifying extreme weather events, shifting precipitation patterns, and accelerating biodiversity loss. This document examines the causes, mechanisms, observed impacts, and potential pathways for addressing climate change.",
    "The Earth's climate has always varied naturally, driven by orbital cycles, solar variability, volcanic eruptions, and feedback mechanisms within the climate system. However, the current episode of warming is occurring at a rate unprecedented in at least the past 2000 years and is primarily driven by anthropogenic emissions of greenhouse gases [2]. Carbon dioxide concentrations in the atmosphere have increased from approximately 280 parts per million in the pre-industrial era to over 420 parts per million today, primarily from fossil fuel combustion and deforestation. Methane and nitrous oxide, though present in smaller concentrations, exert stronger warming effects per molecule. The greenhouse effect, whereby these gases trap outgoing infrared radiation, is well-understood physics dating to work by Fourier, Tyndall, and Arrhenius in the nineteenth century.",
    "Observed Impacts and Projections",
    "The physical impacts of climate change are already observable across the globe. Arctic sea ice extent has declined dramatically, with summer minimum extents approximately 40 percent below 1980 levels. Glaciers worldwide are retreating, threatening freshwater supplies for hundreds of millions of people who depend on glacial meltwater. Sea level has risen approximately 20 centimeters since 1900 and is accelerating, threatening coastal populations and infrastructure [3]. Extreme heat events have become more frequent and intense, with attribution science now capable of quantifying the human influence on specific events. Changing precipitation patterns are intensifying droughts in some regions while increasing flood risk in others. Coral reef systems, which support approximately 25 percent of marine biodiversity, are experiencing mass bleaching events at unprecedented frequency.",
    "Mitigation Strategies",
    "Limiting warming to 1.5 or 2 degrees Celsius above pre-industrial levels, as established in the Paris Agreement, requires rapid and deep reductions in greenhouse gas emissions across all sectors of the economy. The energy transition from fossil fuels to renewable sources represents the central mitigation challenge. Solar and wind power have achieved cost competitiveness with fossil fuels in most markets, and deployment is accelerating globally. However, the pace of transition must dramatically increase to meet climate targets [1]. Electrification of transportation, buildings, and industrial processes, coupled with decarbonization of electricity generation, represents the core technical pathway. Carbon capture and storage technologies may play a role in addressing hard-to-abate industrial emissions. Natural climate solutions including forest protection, restoration, and improved agricultural practices offer significant mitigation potential at relatively low cost.",
    "Adaptation and Resilience",
    "Given the warming already locked in by historical emissions, adaptation to climate impacts is an imperative alongside mitigation. Adaptation encompasses a wide range of strategies including infrastructure hardening against extreme events, agricultural system modification, managed retreat from high-risk coastal areas, heat action plans for vulnerable urban populations, and ecosystem-based approaches that harness natural systems for flood protection and cooling [4]. The differential vulnerability of populations to climate impacts demands attention to climate justice, as communities with least historical responsibility for emissions often face the greatest risks. The economic costs of insufficient adaptation are projected to far exceed the costs of proactive adaptation investment.",
    "International climate governance has evolved significantly since the 1992 UN Framework Convention on Climate Change. The 2015 Paris Agreement established a framework of nationally determined contributions with mechanisms for progressive ambition increase. However, current national pledges remain insufficient to limit warming to agreed targets, creating an 'ambition gap' requiring stronger policy action. Carbon pricing mechanisms including taxes and cap-and-trade systems provide economic incentives for emissions reduction. Sectoral policies targeting energy efficiency, renewable deployment, and clean transportation are being implemented across jurisdictions. Climate finance flows from developed to developing nations remain below commitments.",
    "Climate change demands an unprecedented level of international cooperation and domestic policy transformation. The technical solutions required for deep decarbonization exist and are increasingly cost-competitive. The central challenge is accelerating deployment through appropriate policy frameworks, institutional capacity, and political will [2]. Addressing climate change represents not only an environmental imperative but an economic opportunity, with the clean energy transition projected to create millions of jobs and drive technological innovation. The decisions made in this decade will determine the climate trajectory for centuries.",
    "IPCC","2021","Sixth Assessment Report: The Physical Science Basis","Cambridge University Press",
    "Arrhenius, S.","1896","On the Influence of Carbonic Acid in the Air upon the Temperature of the Ground","Philosophical Magazine vol 41",
    "Oppenheimer, M. et al","2019","Sea Level Rise and Implications for Low Lying Islands Coasts and Communities","IPCC Special Report on Ocean and Cryosphere",
    "UNEP","2021","Adaptation Gap Report 2021","United Nations Environment Programme"
  },
  { "technology",
    "Technology, broadly defined as the application of scientific knowledge for practical purposes, has been the primary driver of human civilizational development from the Paleolithic to the digital age. The contemporary technological landscape, characterized by accelerating innovation in digital systems, biotechnology, materials science, and energy, presents both unprecedented opportunities and complex challenges for individuals, organizations, and societies [1]. This document provides a comprehensive examination of technological development, its economic and social impacts, governance challenges, and future trajectories.",
    "The history of technology is fundamentally the history of human capacity to harness natural forces and materials for productive purposes. Each major technological transition, from agriculture to metallurgy, from steam power to electrification, from telecommunications to computing, has restructured economic systems and social organization in profound ways [2]. The current digital revolution, initiated by the development of the transistor and integrated circuit, has achieved the remarkable feat of continuously doubling computational power per unit cost for over five decades, enabling capabilities that would have seemed miraculous to previous generations. The convergence of digital technologies with biotechnology, nanotechnology, and materials science suggests we may be entering a period of even more rapid and disruptive technological change.",
    "Digital Transformation",
    "Digital transformation refers to the integration of digital technologies into all aspects of organizational and social activity, fundamentally changing how value is created and delivered. Cloud computing provides flexible, scalable access to computational resources, democratizing capabilities previously available only to large enterprises. Mobile technology has extended internet connectivity to billions of people globally, creating new markets and enabling services ranging from financial inclusion through mobile payments to agricultural productivity through weather and market information [3]. The Internet of Things connects physical objects to digital networks, enabling new forms of monitoring, control, and optimization in manufacturing, logistics, agriculture, and urban management. Blockchain and distributed ledger technologies offer new mechanisms for trust and coordination in digital transactions.",
    "Innovation Ecosystems",
    "Technological innovation does not occur in isolation but within ecosystems of organizations, institutions, and networks that collectively generate, diffuse, and apply new knowledge. Research universities serve as fundamental generators of foundational knowledge through basic research, while corporate R&D organizations translate knowledge into products and services [1]. Startup ecosystems, most prominently Silicon Valley but increasingly distributed globally, provide environments for high-risk innovation backed by venture capital. Government laboratories and research programs address challenges where market incentives are insufficient, including defense, public health, and fundamental scientific research. The open source software movement demonstrates that collaborative, non-proprietary models of innovation can generate enormously valuable technological outputs.",
    "Technology and Society",
    "The relationship between technology and society is complex and bidirectional. Technologies shape social practices, power relationships, and cultural norms while simultaneously being shaped by social forces including market incentives, regulatory frameworks, and cultural values [4]. The digital revolution has created new forms of connectivity and community while also enabling surveillance, misinformation, and platform-mediated power concentration. Social media platforms exemplify this ambivalence, facilitating unprecedented global communication while also amplifying polarization and enabling manipulation. Questions of digital access and literacy create new dimensions of inequality as participation in digital economies becomes essential for economic opportunity.",
    "Technology governance encompasses the regulatory frameworks, standards, norms, and institutions through which societies shape technological development and deployment. Intellectual property regimes balance incentives for innovation with access to knowledge. Product safety and liability frameworks ensure technologies meet minimum standards. Competition policy addresses market concentration in technology industries. Data protection regulations govern the collection and use of personal information. The pace of technological change frequently outstrips governance capacity, creating regulatory gaps that may allow harmful applications while potentially constraining beneficial ones.",
    "Technology will continue to transform human society in ways we can only partially anticipate. Navigating this transformation successfully requires investment in the scientific and engineering capabilities that drive innovation, governance frameworks that channel technological development toward beneficial ends, educational systems that prepare workers for changing labor markets, and inclusive policies that ensure the benefits of technological progress are broadly shared [2]. The central challenge is not technology itself but the human institutions and choices that determine how technologies are developed and deployed.",
    "Brynjolfsson, E. & McAfee, A.","2014","The Second Machine Age","W. W. Norton and Company",
    "Diamond, J.","1997","Guns Germs and Steel: The Fates of Human Societies","W. W. Norton and Company",
    "World Bank","2016","World Development Report 2016: Digital Dividends","World Bank Publications",
    "Winner, L.","1980","Do Artifacts Have Politics","Daedalus vol 109 no 1 pp 121-136"
  },
  { "education",
    "Education constitutes the fundamental mechanism through which human societies transmit accumulated knowledge, values, and skills across generations, while simultaneously developing the human capacity for critical thinking, creativity, and civic participation. As the twenty-first century presents unprecedented challenges from technological disruption to climate change, the quality and accessibility of education has never been more consequential for individual flourishing and collective wellbeing [1]. This document examines the theoretical foundations of education, the evidence on effective pedagogical approaches, the digital transformation of learning, equity challenges, and future directions for educational policy and practice.",
    "Educational theory has evolved significantly from early transmission models that conceived of learning as the passive receipt of information from teacher to student. Constructivist theories, drawing on the work of Piaget and Vygotsky, emphasize that learners actively construct understanding through experience and social interaction [2]. Cognitive science research has illuminated the mechanisms of learning including working memory limitations, the importance of retrieval practice for long-term retention, and the role of motivation and emotion in learning outcomes. Sociocultural approaches emphasize the fundamentally social nature of learning and the importance of the broader context including family, community, and institutional culture in shaping educational outcomes. Contemporary evidence-based education draws on randomized controlled trials and rigorous quasi-experimental research to identify effective instructional practices.",
    "Pedagogical Innovation",
    "Evidence-based pedagogical practices increasingly inform educational design. Formative assessment, or the ongoing monitoring of student learning to inform instruction, has been identified as one of the highest-impact interventions available to teachers [3]. Feedback that is specific, timely, and focused on the task rather than the person is particularly effective. Spaced repetition and interleaved practice, informed by cognitive science research on memory consolidation, improve long-term retention compared to massed practice. Collaborative learning structures harness the social dimensions of learning, developing both content knowledge and interpersonal competencies. Project-based learning engages students in extended, authentic investigations that develop deeper understanding and transferable skills.",
    "Technology in Education",
    "Digital technologies offer significant potential to transform educational access and quality, though realizing this potential requires thoughtful implementation rather than mere technology adoption [1]. Adaptive learning platforms adjust the difficulty and presentation of content based on student performance, providing personalized instruction at scale. Massive open online courses have extended access to high-quality educational content globally, though completion rates remain low without supporting structures. Intelligent tutoring systems can provide immediate, individualized feedback traditionally available only through one-on-one tutoring. Virtual and augmented reality enable immersive learning experiences that support spatial reasoning and engagement with abstract concepts.",
    "Educational Equity",
    "Access to quality education remains profoundly unequal both within and across countries, reproducing and amplifying existing social inequalities. Socioeconomic status, race, gender, language, disability, and geography all influence educational access and outcomes [4]. Early childhood education and care, particularly for children from disadvantaged backgrounds, has the highest returns of any educational investment in terms of long-term outcomes. Teacher quality is the most important in-school factor affecting student learning, making teacher recruitment, training, and retention central policy priorities. School funding formulas in many jurisdictions perpetuate inequality by linking school resources to local property tax revenues.",
    "Educational systems face the challenge of preparing learners for a world characterized by technological change, global interdependence, and complex challenges requiring collaborative problem-solving. Twenty-first century competency frameworks emphasize critical thinking, creativity, communication, and collaboration alongside traditional academic knowledge. Curriculum reform debates center on the appropriate balance between foundational knowledge and skills for adaptability and lifelong learning. Assessment systems must evolve beyond measuring discrete knowledge recall to capturing broader competencies. Teacher professional development requires sustained, collaborative models focused on instructional improvement rather than compliance.",
    "Education is simultaneously a fundamental human right, a critical investment in individual and societal development, and a complex system shaped by historical legacies, political contestation, and resource constraints. Improving educational quality and equity requires sustained commitment to evidence-based practice, adequate and equitable resource allocation, strong teacher preparation and support, and governance structures that enable continuous improvement [2]. The potential of education to expand human capability and address pressing global challenges makes it one of the highest-priority domains for policy attention and investment.",
    "Hattie, J.","2009","Visible Learning: A Synthesis of Over 800 Meta-Analyses Relating to Achievement","Routledge",
    "Vygotsky, L.S.","1978","Mind in Society: The Development of Higher Psychological Processes","Harvard University Press",
    "Black, P. & Wiliam, D.","1998","Inside the Black Box: Raising Standards Through Classroom Assessment","Phi Delta Kappan vol 80 no 2",
    "UNESCO","2020","Global Education Monitoring Report 2020: Inclusion and Education","UNESCO Publishing"
  },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
};

static topic_db_t generic = {
    "general",
    "This document provides a comprehensive analysis of the subject under examination. Through systematic investigation drawing on relevant theoretical frameworks and empirical evidence, it aims to illuminate key dimensions of the topic, identify significant patterns and relationships, and derive implications for practice and policy. The analysis is structured to provide both breadth of coverage across major themes and sufficient depth to support informed understanding and decision-making [1].",
    "Understanding this subject requires situating it within its historical and conceptual context. The field has developed through successive phases of theoretical refinement and empirical investigation, with contemporary understanding reflecting accumulated insights from multiple disciplines and methodological traditions [2]. Key conceptual frameworks that inform analysis include systems thinking, which emphasizes the interconnected nature of complex phenomena; stakeholder analysis, which maps the interests and influences of relevant actors; and comparative analysis, which derives insights from examination of variation across cases and contexts.",
    "Theoretical Framework",
    "The theoretical foundations for analyzing this subject draw on multiple disciplinary traditions. Structural approaches examine how macro-level factors and institutional arrangements shape outcomes. Agency-centered approaches attend to the role of individual and collective actors in producing change. Process perspectives trace how phenomena unfold over time through sequences of events and feedback mechanisms. Integrative frameworks seek to synthesize insights across levels of analysis and disciplinary perspectives to provide more comprehensive explanation [3]. The application of these frameworks to empirical evidence enables systematic analysis that goes beyond description to explanation and prediction.",
    "Empirical Analysis",
    "Empirical examination of this subject reveals several significant patterns and findings. First, outcomes vary substantially across cases and contexts, indicating that simple universal explanations are inadequate and that contextual factors play important roles [1]. Second, multiple causal pathways can produce similar outcomes, suggesting equifinality in complex systems. Third, there are significant time lags between causes and effects, requiring longitudinal analysis rather than cross-sectional snapshots. Fourth, feedback mechanisms create nonlinear dynamics that can produce rapid change following periods of apparent stability. These patterns have important implications for both theoretical understanding and practical application.",
    "Policy and Practice Implications",
    "The analysis has several significant implications for policy and practice. First, interventions should be designed with attention to context, recognizing that approaches effective in one setting may not transfer without adaptation [4]. Second, monitoring and evaluation systems should be designed to capture not only intended outcomes but also unintended consequences and contextual factors affecting implementation. Third, stakeholder engagement throughout design and implementation improves both the appropriateness of interventions and the likelihood of effective execution. Fourth, learning systems that enable continuous improvement based on evidence are more effective than one-time implementations.",
    "International and comparative perspectives enrich understanding by highlighting how different institutional arrangements, cultural contexts, and resource levels affect outcomes. Cross-national learning must be undertaken with care, attending to the specific conditions that enabled success in exemplary cases and the potential barriers to transfer. Regional cooperation offers opportunities for shared learning and joint action on challenges that transcend national boundaries. The global dimensions of many contemporary challenges require coordinated international responses alongside national and local action.",
    "This analysis has examined the key dimensions of the subject, drawing on theoretical frameworks and empirical evidence to illuminate significant patterns, relationships, and implications. The findings underscore the complexity of the phenomena under examination and the need for nuanced, context-sensitive approaches to understanding and action [2]. Priority areas for further research include longitudinal analysis of causal mechanisms, comparative studies across diverse contexts, and participatory research that integrates practitioner knowledge with academic expertise.",
    "Smith, J. & Jones, A.","2022","Comprehensive Analysis: Theory and Practice","Academic Press",
    "Johnson, R.","2020","Foundations of Modern Research","Cambridge University Press",
    "Williams, K. et al","2021","Empirical Methods in Contemporary Research","Oxford University Press",
    "Brown, M. & Davis, L.","2019","Evidence-Based Practice and Policy","Sage Publications"
};

void doc_page_write_long(const char* title, const char* type,
                          const char* topic, int pages){
    extern int current_page;
    extern doc_entry_t doc_store[];
    extern int doc_count, active_doc;
    extern void render_doc(void);
    extern void doc_page_save(void);

    if(active_doc<0||active_doc>=DOC_STORE_MAX) return;
    doc_entry_t* d = &doc_store[active_doc];
    char* b = d->body;
    b[0] = 0;
    int m = DOC_BODY_LEN;

    /* Find matching topic */
    topic_db_t* td = &generic;
    for(int i=0;topics[i].keyword;i++){
        const char* k=topics[i].keyword;
        const char* t=topic;
        int match=1;
        while(*k){if(*t!=*k)match=0;t++;k++;}
        if(match){ td=&topics[i]; break; }
        /* partial match */
        const char* p=topic;
        while(*p){
            const char* q=topics[i].keyword;
            int pm=1; const char* pp=p;
            while(*q){if(*pp!=*q)pm=0;pp++;q++;}
            if(pm){td=&topics[i];goto found;}
            p++;
        }
    }
    found:;

    /* ── COVER PAGE ── */
    dcat_safe(b,title,m);
    dcat_safe(b,"\n\n",m);
    dcat_safe(b,type,m);
    dcat_safe(b,"\n\n",m);
    dcat_safe(b,"Submitted to: AIMERANCIA Academic System\n",m);
    dcat_safe(b,"Date: 2026\n\n",m);
    dcat_safe(b,"Word count: approximately ",m);
    /* estimate: pages * 400 words */
    char wc[8]={'0'+pages/10,'0'+pages%10,'0','0',0};
    if(pages<10){wc[0]='0'+pages;wc[1]='0';wc[2]='0';wc[3]='0';wc[4]=0;}
    dcat_safe(b,wc,m);
    dcat_safe(b," words\n\n",m);

    /* ── ABSTRACT ── */
    add_section(b,m,"Abstract",td->intro);

    /* ── TABLE OF CONTENTS ── */
    add_section(b,m,"Table of Contents",
        "1. Introduction\n"
        "2. Background and Context\n"
        "3. Theoretical Framework\n"
        "4. Analysis: Part One\n"
        "5. Analysis: Part Two\n"
        "6. Analysis: Part Three\n"
        "7. Policy and Practice Implications\n"
        "8. Governance and Institutional Dimensions\n"
        "9. Future Directions\n"
        "10. Conclusion\n"
        "References\n"
        "Appendices\n");

    /* ── INTRODUCTION ── */
    add_section(b,m,"1. Introduction",td->intro);

    /* Extend introduction for page count */
    for(int i=1;i<pages/8;i++){
        dcat_safe(b,"The significance of this subject extends across multiple domains of inquiry and practice. Scholars have approached it from diverse disciplinary perspectives, yielding a rich but sometimes fragmented body of knowledge. This document seeks to synthesize key insights while identifying areas of continuing debate and uncertainty. The structure of the analysis reflects an attempt to move from conceptual foundations through empirical examination to practical implications, providing a coherent and useful framework for understanding.\n\n",m);
        dcat_safe(b,"The methodology employed in this analysis draws on systematic review of relevant literature, comparative case analysis, and synthesis of empirical findings from multiple research traditions. Sources have been selected for their rigor, relevance, and representativeness of the broader literature. Where findings conflict, the analysis attempts to identify the conditions under which different conclusions hold rather than simply asserting one view as correct.\n\n",m);
    }

    /* ── BACKGROUND ── */
    add_section(b,m,"2. Background and Context",td->background);

    for(int i=1;i<pages/8;i++){
        dcat_safe(b,"Historical analysis reveals that current patterns reflect accumulated decisions and path dependencies stretching back decades or centuries. Understanding this historical context is essential for interpreting present conditions and anticipating future trajectories. Comparative historical analysis across cases with different developmental pathways illuminates the factors that have produced current divergences and the possibilities for different futures.\n\n",m);
        dcat_safe(b,"The institutional context shapes how actors perceive their interests, what strategies are available to them, and what outcomes are achievable. Formal institutions including laws, regulations, and organizational structures provide the framework within which actors operate, while informal institutions including norms, conventions, and cultural practices profoundly influence behavior even where formal rules are silent or ambiguous.\n\n",m);
    }

    /* ── ANALYSIS SECTIONS ── */
    add_section(b,m,td->analysis1_title,td->analysis1);
    for(int i=1;i<pages/10;i++){
        dcat_safe(b,"Further examination reveals additional dimensions of this phenomenon that deserve attention. The interaction effects between different factors create complex patterns that cannot be understood by examining any single variable in isolation. Quantitative analysis of available data confirms the patterns identified through qualitative case examination, strengthening confidence in the findings while also revealing additional nuances.\n\n",m);
    }

    add_section(b,m,td->analysis2_title,td->analysis2);
    for(int i=1;i<pages/10;i++){
        dcat_safe(b,"The evidence reviewed in this section points toward several conclusions with important theoretical and practical significance. First, the phenomena under examination are more complex and context-dependent than simpler models suggest. Second, feedback mechanisms play a crucial role in producing observed patterns. Third, the distribution of outcomes is highly unequal, with benefits and risks distributed according to pre-existing patterns of advantage and disadvantage.\n\n",m);
    }

    add_section(b,m,td->analysis3_title,td->analysis3);
    for(int i=1;i<pages/10;i++){
        dcat_safe(b,"These findings have significant implications for how we understand the theoretical frameworks discussed earlier. In particular, they suggest that models emphasizing linear causation are insufficient and that more complex, systems-oriented approaches better capture the dynamics at work. The evidence also highlights the importance of temporal factors, as outcomes that appear stable in the short term may be subject to significant change over longer time horizons.\n\n",m);
    }

    /* ── IMPLICATIONS ── */
    add_section(b,m,"7. Policy and Practice Implications",td->implications);
    for(int i=1;i<pages/8;i++){
        dcat_safe(b,"Implementation of the recommended approaches requires attention to several practical considerations. Resource constraints affect the feasibility of different options and require prioritization based on evidence of impact. Stakeholder engagement processes must be designed to be genuinely inclusive rather than merely consultative, incorporating the knowledge and perspectives of those most affected by decisions. Timeline expectations must be calibrated to the pace at which complex systems can be changed.\n\n",m);
        dcat_safe(b,"Monitoring and evaluation systems should be established before implementation begins, with clear indicators aligned to intended outcomes and mechanisms for capturing unintended consequences. Learning loops that translate evaluation findings into program adjustments improve outcomes over time and build the evidence base for future decisions. Independent evaluation provides credibility and reduces the risk of confirmation bias in self-assessment.\n\n",m);
    }

    /* ── CONCLUSION ── */
    add_section(b,m,"10. Conclusion",td->conclusion);
    for(int i=1;i<pages/10;i++){
        dcat_safe(b,"In conclusion, this analysis has examined the key dimensions of the subject from multiple perspectives, drawing on theoretical frameworks and empirical evidence to illuminate significant patterns, relationships, and implications. The complexity of the phenomena requires ongoing research and adaptive management rather than fixed solutions. Commitment to evidence-based practice, continuous learning, and equity must guide future efforts.\n\n",m);
    }

    /* ── REFERENCES ── */
    dcat_safe(b,"References\n\n",m);
    add_citation(b,m,1,td->ref1_author,td->ref1_year,td->ref1_title,td->ref1_journal);
    add_citation(b,m,2,td->ref2_author,td->ref2_year,td->ref2_title,td->ref2_journal);
    add_citation(b,m,3,td->ref3_author,td->ref3_year,td->ref3_title,td->ref3_journal);
    add_citation(b,m,4,td->ref4_author,td->ref4_year,td->ref4_title,td->ref4_journal);
    /* Additional references for longer documents */
    if(pages>=10){
        add_citation(b,m,5,"Anderson, C.","2023","Research Methods and Analysis","Springer");
        add_citation(b,m,6,"Martinez, E. & Chen, L.","2022","Global Perspectives on Contemporary Issues","MIT Press");
        add_citation(b,m,7,"Thompson, R.","2021","Evidence and Policy: Bridging the Gap","Policy Press");
        add_citation(b,m,8,"Garcia, M.","2020","Institutional Dimensions of Complex Problems","Oxford University Press");
    }
    if(pages>=20){
        add_citation(b,m,9,"Patel, S. et al","2022","Quantitative Analysis in Social Research","Sage");
        add_citation(b,m,10,"Nguyen, T.","2021","Comparative Approaches to Policy Analysis","Cambridge University Press");
        add_citation(b,m,11,"O'Brien, K.","2023","Transformative Change: Theory and Practice","Routledge");
        add_citation(b,m,12,"Hassan, A.","2020","Ethics and Governance in Applied Research","Academic Press");
    }

    /* ── APPENDICES ── */
    dcat_safe(b,"\nAppendix A: Methodology\n\n",m);
    dcat_safe(b,"This analysis employed a systematic literature review methodology following PRISMA guidelines. The search strategy included electronic database searches across JSTOR, Web of Science, Google Scholar, and specialist repositories using terms related to the subject and its key dimensions. Inclusion criteria required peer-reviewed publication, English language, and direct relevance to the research questions. Quality assessment used standardized tools appropriate to each study design.\n\n",m);
    dcat_safe(b,"Appendix B: Data Sources\n\n",m);
    dcat_safe(b,"Primary data sources consulted include official statistics from national and international agencies, peer-reviewed research publications, policy documents and evaluations, and reports from recognized expert organizations. Secondary analysis of existing datasets was undertaken where primary data were unavailable. All sources have been assessed for credibility and potential bias.\n\n",m);
}

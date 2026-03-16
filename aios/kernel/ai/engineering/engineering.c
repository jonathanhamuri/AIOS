#include "engineering.h"
#include "../../terminal/terminal.h"
#include "../../graphics/vga.h"

static int sstart(const char* s,const char* p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static int seq(const char* a,const char* b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int slen(const char* s){int n=0;while(*s++)n++;return n;}
static void scopy(char* d,const char* s,int m){
    int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;
}

// Simple integer square root
static int isqrt(int n){
    if(n<=0) return 0;
    int x=n, y=(x+1)/2;
    while(y<x){x=y;y=(x+n/x)/2;}
    return x;
}

// Congo DRC cities database
static city_t cities[] = {
    {"kinshasa",    0,   0,  17000000, 320},
    {"lubumbashi",  1600, 800, 2500000, 1230},
    {"kisangani",   800, 200, 1200000, 420},
    {"mbujimayi",   900, 500, 2000000, 700},
    {"kananga",     700, 600, 1200000, 650},
    {"goma",        1200, 100, 700000, 1490},
    {"bukavu",      1300, 200, 800000, 1500},
    {"matadi",      -200, 100, 400000, 30},
    {"kolwezi",     1400, 700, 500000, 1400},
    {"likasi",      1500, 750, 400000, 1260},
    {0,0,0,0,0}
};

static city_t* find_city(const char* name){
    for(int i=0;cities[i].name;i++){
        if(seq(cities[i].name,name)) return &cities[i];
        // Partial match
        if(sstart(name,cities[i].name)) return &cities[i];
    }
    return 0;
}

static int calc_distance(city_t* a, city_t* b){
    int dx = a->lat_x - b->lat_x;
    int dy = a->lat_y - b->lat_y;
    if(dx<0) dx=-dx;
    if(dy<0) dy=-dy;
    return isqrt(dx*dx + dy*dy);
}

static void print_separator(){
    terminal_print_color("================================================\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

static void print_header(const char* title){
    print_separator();
    terminal_print_color("  ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(title,MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_newline();
    print_separator();
}

static void print_field(const char* label, int value, const char* unit){
    terminal_print_color("  ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(label,MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color(": ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_int(value);
    terminal_print_color(" ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(unit,MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_newline();
}

static void print_field_str(const char* label, const char* value){
    terminal_print_color("  ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(label,MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color(": ",MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(value,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_newline();
}

void plan_railway(const char* from, const char* to){
    city_t* city_a = find_city(from);
    city_t* city_b = find_city(to);

    print_header("AIOS RAILWAY ENGINEERING ANALYSIS");

    terminal_print_color("  Route: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color(from,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_print_color(" --> ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_color(to,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_newline();
    terminal_newline();

    int dist = 0;
    int elev_diff = 0;

    if(city_a && city_b){
        dist = calc_distance(city_a, city_b);
        elev_diff = city_b->elevation - city_a->elevation;
        if(elev_diff<0) elev_diff=-elev_diff;
    } else {
        // Default Congo estimate
        dist = 1700;
        elev_diff = 900;
    }

    // Add 15% for terrain routing
    int track_km = dist + (dist*15/100);

    // Engineering calculations
    int bridges     = track_km / 50;        // bridge every 50km avg
    int tunnels     = elev_diff / 300;       // tunnel per 300m elevation
    int speed_kmh   = 160;                   // high speed train
    int travel_h    = dist / speed_kmh;
    if(travel_h < 1) travel_h = 1;

    // Material calculations
    // Steel: 100 tons/km for rails + 50 tons per bridge
    int steel_tons  = track_km * 100 + bridges * 500;
    // Concrete: 500 tons/km for sleepers + tunnels
    int concrete    = track_km * 500 + tunnels * 10000;
    // Workers: 500/km peak
    int workers     = track_km * 500;
    if(workers > 500000) workers = 500000;

    // Cost model (millions USD)
    // Track: $5M/km, Bridge: $50M each, Tunnel: $100M each
    int cost_track  = track_km * 5;
    int cost_bridge = bridges * 50;
    int cost_tunnel = tunnels * 100;
    int cost_total  = cost_track + cost_bridge + cost_tunnel;
    // Add 30% for infrastructure (stations, power, signaling)
    cost_total = cost_total + (cost_total * 30 / 100);

    // Construction time
    int years = track_km / 100; // 100km/year typical
    if(years < 5) years = 5;

    terminal_print_color("  [ROUTE ANALYSIS]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Direct distance", dist, "km");
    print_field("  Track length", track_km, "km");
    print_field("  Elevation change", elev_diff, "m");
    print_field("  Design speed", speed_kmh, "km/h");
    print_field("  Travel time", travel_h, "hours");
    terminal_newline();

    terminal_print_color("  [INFRASTRUCTURE]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Bridges required", bridges, "structures");
    print_field("  Tunnels required", tunnels, "tunnels");
    print_field("  Stations", track_km/200+2, "major stations");
    terminal_newline();

    terminal_print_color("  [MATERIALS]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Steel rails", steel_tons, "tons");
    print_field("  Concrete", concrete, "tons");
    print_field("  Locomotives", track_km/500+5, "units");
    print_field("  Passenger cars", track_km/100+20, "units");
    terminal_newline();

    terminal_print_color("  [WORKFORCE]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Peak workers", workers, "workers");
    print_field("  Engineers needed", workers/100, "engineers");
    print_field("  Construction time", years, "years");
    terminal_newline();

    terminal_print_color("  [COST ESTIMATE]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Track cost", cost_track, "M USD");
    print_field("  Bridge cost", cost_bridge, "M USD");
    print_field("  Tunnel cost", cost_tunnel, "M USD");
    print_field("  TOTAL COST", cost_total, "M USD");
    terminal_newline();

    // Feasibility score
    int feasibility = 70;
    if(cost_total > 10000) feasibility -= 20;
    if(tunnels > 5) feasibility -= 10;
    if(dist > 2000) feasibility -= 10;
    if(feasibility < 0) feasibility = 10;

    terminal_print_color("  [FEASIBILITY]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color("  Score: ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
    terminal_print_int(feasibility);
    terminal_print_color("%  ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));

    if(feasibility >= 70)
        terminal_print_color("VIABLE - Recommended\n",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    else if(feasibility >= 50)
        terminal_print_color("POSSIBLE - Needs study\n",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    else
        terminal_print_color("CHALLENGING - High risk\n",
            MAKE_COLOR(COLOR_BRED,COLOR_BLACK));

    terminal_newline();

    // Draw route visualization
    if(vga_active){
        vga_rectfill(0,14,320,165,0);
        // Draw simple route map
        vga_rectfill(10,80,8,8,3);   // city A
        vga_rectfill(280,80,8,8,3);  // city B
        // Draw track line
        int steps = 20;
        for(int i=0;i<steps;i++){
            int x = 10 + i*(270/steps);
            int y = 80 + (i%3)-1; // slight variation
            vga_putpixel(x,y+4,10);
            vga_putpixel(x,y+5,10);
        }
        // Labels
        vga_drawstring(2,70,from,14,0);
        vga_drawstring(260,70,to,14,0);
        vga_drawstring(100,60,"RAILWAY ROUTE",11,0);
        // Cost bar
        vga_drawstring(2,100,"Cost:",11,0);
        int bar = cost_total/500;
        if(bar>200) bar=200;
        vga_rectfill(40,98,bar,8,feasibility>60?2:4);
        vga_drawstring(2,112,"Time:",11,0);
        vga_rectfill(40,110,years*8,8,14);
    }

    print_separator();
    terminal_print_color("  Generated by AIOS Engineering Engine\n",
        MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
    print_separator();
}

void plan_satellite(const char* name, int orbit_km, int mass_kg){
    print_header("AIOS SATELLITE LAUNCH ANALYSIS");

    if(orbit_km <= 0) orbit_km = 550;   // LEO default
    if(mass_kg <= 0) mass_kg = 1000;

    // Orbital mechanics
    // v = sqrt(GM/r), GM = 3.986e14, r = 6371000 + orbit_m
    // Simplified: LEO ~7800 m/s, GEO ~3070 m/s
    int velocity_ms;
    const char* orbit_type;
    if(orbit_km < 2000){
        velocity_ms = 7800 - orbit_km/10;
        orbit_type = "LEO (Low Earth Orbit)";
    } else if(orbit_km < 35000){
        velocity_ms = 5000 - orbit_km/100;
        orbit_type = "MEO (Medium Earth Orbit)";
    } else {
        velocity_ms = 3070;
        orbit_type = "GEO (Geostationary)";
    }

    // Tsiolkovsky rocket equation simplified
    // Delta-v needed: orbital velocity + gravity losses (~1500 m/s)
    int delta_v = velocity_ms + 1500;
    // Fuel mass ratio (exhaust velocity ~3000 m/s for kerosene)
    // m_fuel = m_payload * (e^(dv/ve) - 1)
    // Simplified: fuel ~= payload * delta_v / 2000
    int fuel_tons = (mass_kg * delta_v / 2000000) + 50;
    int stages = 2;
    if(orbit_km > 35000) stages = 3;

    // Cost model
    int launch_cost = mass_kg * 5000 / 1000; // $5000/kg to LEO
    if(orbit_km > 35000) launch_cost = mass_kg * 15000 / 1000;
    int sat_cost = mass_kg * 50; // $50k/kg satellite cost

    terminal_print_color("  Mission: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color(name,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_newline();
    terminal_newline();

    terminal_print_color("  [ORBITAL PARAMETERS]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field_str("  Orbit type", orbit_type);
    print_field("  Altitude", orbit_km, "km");
    print_field("  Orbital velocity", velocity_ms, "m/s");
    print_field("  Orbital period", (int)(2*3*orbit_km/velocity_ms+90), "minutes");
    terminal_newline();

    terminal_print_color("  [ROCKET PARAMETERS]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Payload mass", mass_kg, "kg");
    print_field("  Fuel required", fuel_tons, "tons");
    print_field("  Rocket stages", stages, "stages");
    print_field("  Delta-V required", delta_v, "m/s");
    terminal_newline();

    terminal_print_color("  [COST ESTIMATE]\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Launch cost", launch_cost, "K USD");
    print_field("  Satellite cost", sat_cost, "K USD");
    print_field("  Total mission", launch_cost+sat_cost, "K USD");
    terminal_newline();

    print_separator();
}

void plan_bridge(const char* location, int span_m){
    print_header("AIOS BRIDGE ENGINEERING ANALYSIS");

    if(span_m <= 0) span_m = 500;

    // Bridge type selection
    const char* bridge_type;
    if(span_m < 100) bridge_type = "Beam Bridge";
    else if(span_m < 300) bridge_type = "Arch Bridge";
    else if(span_m < 1000) bridge_type = "Cable-Stayed Bridge";
    else bridge_type = "Suspension Bridge";

    // Material calculations
    int steel_tons = span_m * span_m / 100;
    int concrete_tons = span_m * 800;
    int cables = span_m > 300 ? span_m/20 : 0;
    int workers = span_m * 10;
    int years = span_m / 200 + 2;
    int cost = span_m * 50; // $50k/m rough estimate

    terminal_print_color("  Location: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    terminal_print_color(location,MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
    terminal_newline();terminal_newline();

    terminal_print_color("  [DESIGN]\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field_str("  Bridge type", bridge_type);
    print_field("  Main span", span_m, "meters");
    print_field("  Design load", span_m*2, "tons");
    terminal_newline();

    terminal_print_color("  [MATERIALS]\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Steel", steel_tons, "tons");
    print_field("  Concrete", concrete_tons, "tons");
    if(cables>0) print_field("  Cable sets", cables, "pairs");
    terminal_newline();

    terminal_print_color("  [CONSTRUCTION]\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    print_field("  Workers", workers, "peak");
    print_field("  Duration", years, "years");
    print_field("  Est. cost", cost, "K USD");
    terminal_newline();

    print_separator();
}

int engineering_handle(const char* input){
    // Railway planning
    if(sstart(input,"plan railway from ")||
       sstart(input,"railway from ")||
       sstart(input,"build railway from ")||
       sstart(input,"train from ")){

        const char* p = input;
        if(sstart(p,"plan railway from ")) p+=18;
        else if(sstart(p,"railway from ")) p+=13;
        else if(sstart(p,"build railway from ")) p+=19;
        else if(sstart(p,"train from ")) p+=11;

        // Parse "X to Y"
        char from[32]={0}, to[32]={0};
        int i=0;
        while(p[i]&&!sstart(p+i," to ")&&i<31){
            from[i]=p[i];i++;
        }
        from[i]=0;
        if(sstart(p+i," to ")){
            const char* q=p+i+4;
            int j=0;
            while(q[j]&&j<31){to[j]=q[j];j++;}
            to[j]=0;
        }

        plan_railway(from[0]?from:"kinshasa",
                     to[0]?to:"lubumbashi");
        return 1;
    }

    // Satellite planning
    if(sstart(input,"launch satellite")||
       sstart(input,"plan satellite")||
       sstart(input,"build satellite")){
        plan_satellite("AIOS-SAT-1", 550, 1000);
        return 1;
    }

    // Bridge planning
    if(sstart(input,"plan bridge")||
       sstart(input,"build bridge")||
       sstart(input,"bridge over")){
        const char* loc = "Congo River";
        if(sstart(input,"bridge over ")) loc=input+12;
        plan_bridge(loc, 800);
        return 1;
    }

    // Feasibility check
    if(sstart(input,"feasibility of ")||
       sstart(input,"is it feasible to ")||
       sstart(input,"can we build ")){
        terminal_print_color("[ENGINEERING] Analyzing feasibility...\n",
            MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print("Use: plan railway from X to Y\n");
        terminal_print("     launch satellite\n");
        terminal_print("     plan bridge over X\n");
        return 1;
    }

    return 0;
}

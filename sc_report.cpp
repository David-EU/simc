// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// print_action ==============================================================

static void print_action( FILE* file, stats_t* s )
{
  if( s -> total_dmg == 0 ) return;

  double total_dmg;

  if( s -> player -> is_pet() )
  {
    total_dmg = s -> player -> cast_pet() -> owner ->  total_dmg;
  }
  else 
  {
    total_dmg = s -> player -> total_dmg;
  }

  fprintf( file, 
           "    %-20s  Count=%5.1f|%4.1fsec  DPE=%5.0f|%2.0f%%  DPET=%5.0f  DPR=%6.1f", 
           s -> name_str.c_str(),
           s -> num_executes,
           s -> frequency,
           s -> dpe, 
           s -> total_dmg * 100.0 / total_dmg, 
           s -> dpet,
           s -> dpr );

  fprintf( file, "  Miss=%.1f%%", s -> execute_results[ RESULT_MISS ].count * 100.0 / s -> num_executes );
      
  if( s -> execute_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    fprintf( file, "  Hit=%4.0f", s -> execute_results[ RESULT_HIT ].avg_dmg );
  }
  if( s -> execute_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    fprintf( file, 
             "  Crit=%5.0f|%5.0f|%.1f%%", 
             s -> execute_results[ RESULT_CRIT ].avg_dmg, 
             s -> execute_results[ RESULT_CRIT ].max_dmg, 
             s -> execute_results[ RESULT_CRIT ].count * 100.0 / s -> num_executes );
  }
  if( s -> execute_results[ RESULT_GLANCE ].avg_dmg > 0 )
  {
    fprintf( file, 
             "  Glance=%4.0f|%.1f%%", 
             s -> execute_results[ RESULT_GLANCE ].avg_dmg, 
             s -> execute_results[ RESULT_GLANCE ].count * 100.0 / s -> num_executes );
  }
  if( s -> execute_results[ RESULT_DODGE ].count > 0 )
  {
    fprintf( file, 
             "  Dodge=%.1f%%", 
             s -> execute_results[ RESULT_DODGE ].count * 100.0 / s -> num_executes );
  }

  if( s -> num_ticks > 0 ) fprintf( file, "  TickCount=%.0f", s -> num_ticks );

  if( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    fprintf( file, 
             "  Tick=%.0f", s -> tick_results[ RESULT_HIT ].avg_dmg );
  }
  if( s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    fprintf( file, 
             "  CritTick=%.0f|%.0f|%.1f%%", 
             s -> tick_results[ RESULT_CRIT ].avg_dmg, 
             s -> tick_results[ RESULT_CRIT ].max_dmg, 
             s -> tick_results[ RESULT_CRIT ].count * 100.0 / s -> num_ticks );
  }

  fprintf( file, "\n" );
}

// print_actions =============================================================

static void print_actions( FILE* file, player_t* p )
{
  fprintf( file, "  Actions:\n" );

  for( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if( s -> total_dmg > 0 )
    {
      print_action( file, s );
    }
  }

  for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;
    for( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if( s -> total_dmg > 0 )
      {
        if( first )
        {
          fprintf( file, "   %s\n", pet -> name_str.c_str() );
          first = false;
        }
        print_action( file, s );
      }
    }
  }
}

// print_core_stats ===========================================================

static void print_core_stats( FILE* file, player_t* p )
{
  fprintf( file, 
           "  Core Stats:  strength=%.0f  agility=%.0f  stamina=%.0f  intellect=%.0f  spirit=%.0f  health=%.0f  mana=%.0f\n", 
           p -> strength(), p -> agility(), p -> stamina(), p -> intellect(), p -> spirit(),
           p -> resource_max[ RESOURCE_HEALTH ], p -> resource_max[ RESOURCE_MANA ] );
}

// print_spell_stats ==========================================================

static void print_spell_stats( FILE* file, player_t* p )
{
  fprintf( file, 
           "  Spell Stats:  power=%.0f  hit=%.1f%%  crit=%.1f%%  penetration=%.0f  haste=%.1f%%  mp5=%.0f\n", 
           p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(),
           p -> composite_spell_hit()  * 100.0, 
           p -> composite_spell_crit() * 100.0,
           p -> composite_spell_penetration(),
           ( 1.0 / p -> spell_haste - 1 ) * 100.0,
           p -> initial_mp5 );
}

// print_attack_stats =========================================================

static void print_attack_stats( FILE* file, player_t* p )
{
  fprintf( file, 
           "  Attack Stats  power=%.0f  hit=%.1f%%  crit=%.1f%%  expertise=%.1f  penetration%.1f  haste=%.1f%%\n", 
           p -> composite_attack_power() * p -> composite_attack_power_multiplier(),
           p -> composite_attack_hit()         * 100.0, 
           p -> composite_attack_crit()        * 100.0, 
           p -> composite_attack_expertise()   * 100.0,
           p -> composite_attack_penetration() * 100.0,
           ( 1.0 / p -> attack_haste - 1 )     * 100.0 );
}

// print_defense_stats =======================================================

static void print_defense_stats( FILE* file, player_t* p )
{
  fprintf( file, 
           "  Defense Stats:  armor=%.0f\n", 
           p -> composite_armor() );
}

// print_gains ===============================================================

static void print_gains( FILE* file, sim_t* sim )
{
  fprintf( file, "\nGains:\n" );

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;

    bool first=true;
    for( gain_t* g = p -> gain_list; g; g = g -> next )
    {
      if( g -> actual > 0 ) 
      {
        if( first )
        {
          fprintf( file, "\n    %s:\n", p -> name() );
          first = false;
        }
        fprintf( file, "        %s=%.1f", g -> name(), g -> actual );
        double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
        if( overflow_pct > 1.0 ) fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
        fprintf( file, "\n" );
      }
    }
  }
}

// print_procs ================================================================

static void print_procs( FILE* file, sim_t* sim )
{
  fprintf( file, "\nProcs:\n" );

  for( player_t* player = sim -> player_list; player; player = player -> next )
  {
    if( player -> quiet ) 
      continue;

    bool first=true;
    for( proc_t* p = player -> proc_list; p; p = p -> next )
    {
      if( p -> count > 0 ) 
      {
        if( first )
        {
          fprintf( file, "\n    %s:\n", player -> name() );
          first = false;
        }
        fprintf( file, "        %s=%.1f|%.2fsec\n", p -> name(), p -> count, p -> frequency );
      }
    }
  }
}

// print_uptime ===============================================================

static void print_uptime( FILE* file, sim_t* sim )
{
  fprintf( file, "\nUp-Times:\n" );

  bool first=true;
  for( uptime_t* u = sim -> target -> uptime_list; u; u = u -> next )
  {
    if( u -> up > 0 ) 
    {
      if( first )
      {
        fprintf( file, "\n    Global:\n" );
        first = false;
      }
      fprintf( file, "        %4.1f%% : %s\n", u -> percentage(), u -> name() );
    }
  }

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;

    first=true;
    for( uptime_t* u = p -> uptime_list; u; u = u -> next )
    {
      if( u -> up > 0 ) 
      {
        if( first )
        {
          fprintf( file, "\n    %s:\n", p -> name() );
          first = false;
        }
        fprintf( file, "        %4.1f%% : %s\n", u -> percentage(), u -> name() );
      }
    }
  }
}

// print_waiting ===============================================================

static void print_waiting( FILE* file, sim_t* sim )
{
  fprintf( file, "\nWaiting:\n" );

  bool nobody_waits = true;

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;
    
    if( p -> total_waiting )
    {
      nobody_waits = false;
      fprintf( file, "    %4.1f%% : %s\n", 100.0 * p -> total_waiting / p -> total_seconds,  p -> name() );
    }
  }

  if( nobody_waits ) fprintf( file, "    All players active 100%% of the time.\n" );
}

// print_performance ==========================================================

static void print_performance( FILE* file, sim_t* sim )
{
  fprintf( file, 
           "\nBaseline Performance:\n"
           "  TotalEvents   = %d\n"
           "  MaxEventQueue = %d\n"
           "  SimSeconds    = %.0f\n"
           "  CpuSeconds    = %.3f\n"
           "  SpeedUp       = %.0f\n", 
           sim -> total_events_processed,
           sim -> max_events_remaining,
           sim -> iterations * sim -> total_seconds,
           sim -> elapsed_cpu_seconds,
           sim -> iterations * sim -> total_seconds / sim -> elapsed_cpu_seconds );
}

// print_scale_factors ========================================================

static void print_scale_factors( FILE* file, sim_t* sim )
{
  if( ! sim -> scaling -> calculate_scale_factors ) return;

  fprintf( file, "\nScale Factors:\n" );

  int num_players = sim -> players_by_name.size();

  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    fprintf( file, "  %-25s", p -> name() );

    for( int j=0; j < STAT_MAX; j++ )
    {
      if( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        fprintf( file, "  %s=%.2f", util_t::stat_type_abbrev( j ), p -> scaling.get_stat( j ) );
      }
    }
    fprintf( file, "\n" );
  }
}

// print_html_menu_definition ================================================

static void print_html_menu_definition( FILE*  file,
                                        sim_t* sim )
{
  fprintf( file, "<script type=\"text/javascript\">\n"
           "function hideElement(el) {if (el) el.style.display='none';}\n"
           "function showElement(el) {if (el) el.style.display='';}\n"
           "function hideElements(els) {if (els) {"
           "for (var i = 0; i < els.length; i++) hideElement(els[i]);"
           "}}\n"
           "function showElements(els) {if (els) {"
           "for (var i = 0; i < els.length; i++) showElement(els[i]);"
           "}}\n"
           "</script>\n" );
}

// print_html_menu_triggers ==================================================

static void print_html_menu_triggers( FILE*  file,
                                      sim_t* sim )
{
  fprintf( file, "<a href=\"javascript:hideElement(document.getElementById('trigger_menu'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElement(document.getElementById('trigger_menu'));\">+</a> " );
  fprintf( file, "Menu\n" );

  fprintf( file, "<div id=\"trigger_menu\" style=\"display:none;\">" );
  
  fprintf( file, "<a href=\"javascript:hideElements(document.getElementById('players').getElementsByTagName('img'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementById('players').getElementsByTagName('img'));\">+</a> " );
  fprintf( file, "All Charts<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_dpet'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_dpet'));\">+</a> " );
  fprintf( file, "DamagePerExecutionTime<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_uptimes'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_uptimes'));\">+</a> " );
  fprintf( file, "Up-Times and Procs<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_sources'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_sources'));\">+</a> " );
  fprintf( file, "Damage Sources<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_gains'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_gains'));\">+</a> " );
  fprintf( file, "Resource Gains<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_dps_timeline'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_dps_timeline'));\">+</a> " );
  fprintf( file, "DPS Timeline<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_dps_distribution'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_dps_distribution'));\">+</a> " );
  fprintf( file, "DPS Distribution<br/>\n" );

  fprintf( file, "<a href=\"javascript:hideElements(document.getElementsByName('chart_resource_timeline'));\">-</a> " );
  fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_resource_timeline'));\">+</a> " );
  fprintf( file, "Resource Timeline<br/>\n" );
  
  fprintf( file, "</div>" );//trigger_menu
  
  fprintf( file, "<hr>\n" );
}

// print_html_raid ===========================================================

static void print_html_raid( FILE*  file,
                             sim_t* sim )
{
  fprintf( file, "<h1>Raid</h1>\n" );

  assert( sim ->  dps_charts.size() == 
          sim -> gear_charts.size() );

  int count = sim -> dps_charts.size();
  for( int i=0; i < count; i++ )
  {
    fprintf( file, "\n<! DPS Ranking: >\n" );
    fprintf( file, "<img src=\"%s\" />\n", sim -> dps_charts[ i ].c_str() );

    fprintf( file, "\n<! Gear Overview: >\n" );
    fprintf( file, "<img src=\"%s\" />", sim -> gear_charts[ i ].c_str() );
  }

  if( ! sim -> downtime_chart.empty() )
  {
    fprintf( file, "\n<! Raid Downtime: >\n" );
    fprintf( file, "<img src=\"%s\" />", sim -> downtime_chart.c_str() );
  }

  if( ! sim -> uptimes_chart.empty() )
  {
    fprintf( file, "\n<! Global Up-Times: >\n" );
    fprintf( file, "<img src=\"%s\" />", sim -> uptimes_chart.c_str() );
  }

  count = sim -> dpet_charts.size();
  for( int i=0; i < count; i++ )
  {
    fprintf( file, "\n<! Raid Damage Per Execute Time: >\n" );
    fprintf( file, "<img src=\"%s\" />", sim -> dpet_charts[ i ].c_str() );
  }
  fprintf( file, "<hr>\n" );
}

// print_html_scale_factors ===================================================

static void print_html_scale_factors( FILE*  file,
                                      sim_t* sim )
{
  if( ! sim -> scaling -> calculate_scale_factors ) return;

  fprintf( file, "<h1>DPS Scale Factors (dps increase per unit stat)</h1>\n" );

  fprintf( file, "<TABLE BORDER CELLPADDING=4>\n" );
  fprintf( file, "<TR> <TH>profile</TH>" );
  for( int i=0; i < STAT_MAX; i++ )
  {
    if( sim -> scaling -> stats.get_stat( i ) != 0 )
    {
      fprintf( file, " <TH>%s</TH>", util_t::stat_type_abbrev( i ) );
    }
  }
  fprintf( file, " <TH>lootrank</TH> <TH>wowhead</TH> </TR>\n" );

  std::string buffer;
  int num_players = sim -> players_by_name.size();

  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    fprintf( file, "<TR> <TD>%s</TD>", p -> name() );

    for( int j=0; j < STAT_MAX; j++ )
    {
      if( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        fprintf( file, " <TD>%.2f</TD>", p -> scaling.get_stat( j ) );
      }
    }

    fprintf( file, " <TD><a href=\"%s\"> lootrank</a></TD>", p -> gear_weights_lootrank_link.c_str() );
    fprintf( file, " <TD><a href=\"%s\"> wowhead </a></TD>", p -> gear_weights_wowhead_link.c_str() );

    fprintf( file, " </TR>\n" );
  }
  fprintf( file, "</TABLE>\n" );

  fprintf( file, "<hr>\n" );
}

// print_html_player =========================================================

static void print_html_player( FILE*     file,
                               player_t* p )
{
  fprintf( file, "<h1><a href=\"%s\">%s</a></h1>\n", p -> talents_str.c_str(), p -> name() );

  if( ! p -> action_dpet_chart.empty() )
  {
    fprintf( file, "\n<! %s Damage Per Execute Time: >\n", p -> name() );
    fprintf( file, "<img name=\"chart_dpet\" src=\"%s\" />", p -> action_dpet_chart.c_str() );
  }

  if( ! p -> uptimes_and_procs_chart.empty() )
  {
    fprintf( file, "\n<! %s Up-Times and Procs: >\n", p -> name() );
    fprintf( file, "<img name=\"chart_uptimes\" src=\"%s\" />", p -> uptimes_and_procs_chart.c_str() );
  }
  fprintf( file, "<br>\n" );

  if( ! p -> action_dmg_chart.empty() )
  {
    fprintf( file, "\n<! %s Damage Sources: >\n", p -> name() );
    fprintf( file, "<img name=\"chart_sources\" src=\"%s\" />", p -> action_dmg_chart.c_str() );
  }
    
  if( ! p -> gains_chart.empty() )
  {
    fprintf( file, "\n<! %s Resource Gains: >\n", p -> name() );
    fprintf( file, "<img name=\"chart_gains\" src=\"%s\" />", p -> gains_chart.c_str() );
  }
  fprintf( file, "<br>\n" );

  fprintf( file, "\n<! %s DPS Timeline: >\n", p -> name() );
  fprintf( file, "<img name=\"chart_dps_timeline\" src=\"%s\" />\n", p -> timeline_dps_chart.c_str() );

  fprintf( file, "\n<! %s DPS Distribution: >\n", p -> name() );
  fprintf( file, "<img name=\"chart_dps_distribution\" src=\"%s\" />\n", p -> distribution_dps_chart.c_str() );

  fprintf( file, "\n<! %s Resource Timeline: >\n", p -> name() );
  fprintf( file, "<img name=\"chart_resource_timeline\" src=\"%s\" />\n", p -> timeline_resource_chart.c_str() );

  fprintf( file, "<hr>\n" );
}

// print_html_text ===========================================================

static void print_html_text( FILE*  file,
                             sim_t* sim )
{
  fprintf( file, "<h1>Raw Text Output</h1>\n" );

  fprintf( file, "%s",
           "<ul>\n"
           " <li><b>DPS=Num:</b> <i>Num</i> is the <i>damage per second</i>\n"
           " <li><b>DPR=Num:</b> <i>Num</i> is the <i>damage per resource</i>\n"
           " <li><b>RPS=Num1/Num2:</b> <i>Num1</i> is the <i>resource consumed per second</i> and <i>Num2</i> is the <i>resource regenerated per second</i>\n"
           " <li><b>Count=Num|Time:</b> <i>Num</i> is number of casts per fight and <i>Time</i> is average time between casts\n"
           " <li><b>DPE=Num:</b> <i>Num</i> is the <i>damage per execute</i>\n"
           " <li><b>DPET=Num:</b> <i>Num</i> is the <i>damage per execute</i> divided by the <i>time to execute</i> (this value includes GCD costs and Lag in the calculation of <i>time to execute</i>)\n"
           " <li><b>Hit=Num:</b> <i>Num</i> is the average damage per non-crit hit\n"
           " <li><b>Crit=Num1|Num2|Pct:</b> <i>Num1</i> is average crit damage, <i>Num2</i> is the max crit damage, and <i>Pct</i> is the percentage of crits <i>per execute</i> (not <i>per hit</i>)\n"
           " <li><b>Tick=Num:</b> <i>Num</i> is the average tick of damage for the <i>damage over time</i> portion of actions\n"
           " <li><b>Up-Time:</b> This is <i>not</i> the percentage of time the buff/debuff is present, but rather the ratio of <i>actions it affects</i> over <i>total number of actions it could affect</i>.  If spell S is cast 10 times and buff B is present for 3 of those casts, then buff B has an up-time of 30%.\n"
           " <li><b>Waiting</b>: This is percentage of total time not doing anything (except auto-attack in the case of physical dps classes).  This can occur because the player is resource constrained (Mana, Energy, Rage) or cooldown constrained (as in the case of Enhancement Shaman).\n"
           "</ul>\n" );

  fprintf( file, "<pre>\n" );
  report_t::print_text( file, sim );
  fprintf( file, "</pre>\n" );
}

// print_wiki_raid ===========================================================

static void print_wiki_raid( FILE*  file,
                             sim_t* sim )
{
  fprintf( file, "----\n" );
  fprintf( file, "----\n" );
  fprintf( file, "----\n" );
  fprintf( file, "= Raid Charts =\n" );

  assert( sim ->  dps_charts.size() == 
          sim -> gear_charts.size() );

  int count = sim -> dps_charts.size();
  for( int i=0; i < count; i++ )
  {
    fprintf( file, "|| %s&dummy=dummy.png || %s&dummy=dummy.png ||\n", 
             sim -> dps_charts[ i ].c_str(), sim -> gear_charts[ i ].c_str() );
  }

  std::string raid_downtime = "No chart for Raid Down-Time";
  std::string raid_uptimes  = "No chart for Raid Up-Times";

  if( ! sim -> downtime_chart.empty() )
  {
    raid_downtime = sim -> downtime_chart;
    raid_downtime += "&dummy=dummy.png";
  }
  if( ! sim -> uptimes_chart.empty() )
  {
    raid_uptimes = sim -> uptimes_chart;
    raid_uptimes += "&dummy=dummy.png";
  }

  fprintf( file, "|| %s || %s ||\n", raid_downtime.c_str(), raid_uptimes.c_str() );

  count = sim -> dpet_charts.size();
  for( int i=0; i < count; i++ )
  {
    std::string raid_dpet = sim -> dpet_charts[ i ] + "&dummy=dummy.png";
    fprintf( file, "|| %s ", raid_dpet.c_str() );
    if( ++i < count )
    {
      raid_dpet = sim -> dpet_charts[ i ] + "&dummy=dummy.png";
      fprintf( file, "|| %s ", raid_dpet.c_str() );
    }
    fprintf( file, "||\n" );
  }

  fprintf( file, "\n" );
}

// print_wiki_scale_factors ===================================================

static void print_wiki_scale_factors( FILE*  file,
                                      sim_t* sim )
{
  if( ! sim -> scaling -> calculate_scale_factors ) return;

  fprintf( file, "----\n" );
  fprintf( file, "----\n" );
  fprintf( file, "----\n" );
  fprintf( file, "== DPS Scale Factors (dps increase per unit stat) ==\n" );

  fprintf( file, "|| ||" );
  for( int i=0; i < STAT_MAX; i++ )
  {
    if( sim -> scaling -> stats.get_stat( i ) != 0 )
    {
      fprintf( file, " %s ||", util_t::stat_type_abbrev( i ) );
    }
  }
  fprintf( file, " lootrank || wowhead ||\n" );

  std::string buffer;
  int num_players = sim -> players_by_name.size();

  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    fprintf( file, "|| %-25s ||", p -> name() );

    for( int j=0; j < STAT_MAX; j++ )
    {
      if( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
	fprintf( file, " %.2f ||", p -> scaling.get_stat( j ) );
      }
    }

    fprintf( file, " [%s lootrank] ||", p -> gear_weights_lootrank_link.c_str() );
    fprintf( file, " [%s wowhead ] ||", p -> gear_weights_wowhead_link.c_str() );

    fprintf( file, "\n" );
  }
}

// print_wiki_player =========================================================

static void print_wiki_player( FILE*     file,
                               player_t* p )
{
  std::string action_dpet       = "No chart for Damage Per Execute Time";
  std::string uptimes_and_procs = "No chart for Up-Times and Procs";
  std::string action_dmg        = "No chart for Damage Sources";
  std::string gains             = "No chart for Resource Gains";
  std::string timeline_dps      = "No chart for DPS Timeline";
  std::string distribution_dps  = "No chart for DPS Distribution";
  std::string timeline_resource = "No chart for Resource Timeline";

  if( ! p -> action_dpet_chart.empty() )
  {
    action_dpet = p -> uptimes_and_procs_chart;
    action_dpet += "&dummy=dummy.png";
  }
  if( ! p -> uptimes_and_procs_chart.empty() )
  {
    uptimes_and_procs = p -> uptimes_and_procs_chart;
    uptimes_and_procs += "&dummy=dummy.png";
  }
  if( ! p -> action_dmg_chart.empty() )
  {
    action_dmg = p -> action_dmg_chart;
    action_dmg += "&dummy=dummy.png";
  }
  if( ! p -> gains_chart.empty() )
  {
    gains = p -> gains_chart;
    gains += "&dummy=dummy.png";
  }
  if( ! p -> timeline_dps_chart.empty() )
  {
    timeline_dps = p -> timeline_dps_chart;
    timeline_dps += "&dummy=dummy.png";
  }
  if( ! p -> distribution_dps_chart.empty() )
  {
    distribution_dps = p -> distribution_dps_chart;
    distribution_dps += "&dummy=dummy.png";
  }
  if( ! p -> timeline_resource_chart.empty() )
  {
    timeline_resource = p -> timeline_resource_chart;
    timeline_resource += "&dummy=dummy.png";
  }

  fprintf( file, "\n" );
  fprintf( file, "----\n" );
  fprintf( file, "----\n" );
  fprintf( file, "----\n" );
  fprintf( file, "= %s =\n", p -> name() );
  fprintf( file, "[%s Talents]\n", p -> talents_str.c_str() );
  fprintf( file, "\n" );
  fprintf( file, "|| %s || %s ||\n", action_dpet.c_str(), uptimes_and_procs.c_str() );
  fprintf( file, "|| %s || %s ||\n", action_dmg.c_str(),  gains.c_str() );
  fprintf( file, "|| %s || %s ||\n", timeline_dps.c_str(), distribution_dps.c_str() );
  fprintf( file, "|| %s || ||\n", timeline_resource.c_str() );
}

// print_wiki_text ===========================================================

static void print_wiki_text( FILE*  file,
                             sim_t* sim )
{
  fprintf( file, "%s",
           "\n"
           "= Raw Text Output =\n"
           "*Super-Secret Decoder Ring (remember that these values represent the average over all iterations)*\n"
           " * *DPS=Num:* _Num_ is the _damage per second_\n"
           " * *DPR=Num:* _Num_ is the _damage per resource_\n"
           " * *RPS=Num1/Num2:* _Num1_ is the _resource consumed per second_ and _Num2_ is the _resource regenerated per second_\n"
           " * *Count=Num|Time:* _Num_ is number of casts per fight and _Time_ is average time between casts\n"
           " * *DPE=Num:* _Num_ is the _damage per execute_\n"
           " * *DPET=Num:* _Num_ is the _damage per execute_ divided by the _time to execute_ (this value includes GCD costs and Lag in the calculation of _time to execute_)\n"
           " * *Hit=Num:* _Num_ is the average damage per non-crit hit\n"
           " * *Crit=Num1|Num2|Pct:* _Num1_ is average crit damage, _Num2_ is the max crit damage, and _Pct_ is the percentage of crits _per execute_ (not _per hit_)\n"
           " * *Tick=Num:* _Num_ is the average tick of damage for the _damage over time_ portion of actions\n"
           " * *Up-Time:* This is _not_ the percentage of time the buff/debuff is present, but rather the ratio of _actions it affects_ over _total number of actions it could affect_.  If spell S is cast 10 times and buff B is present for 3 of those casts, then buff B has an up-time of 30%.\n"
           " * *Waiting*: This is percentage of total time not doing anything (except auto-attack in the case of physical dps classes).  This can occur because the player is resource constrained (Mana, Energy, Rage) or cooldown constrained (as in the case of Enhancement Shaman).\n"
           "\n" );

  fprintf( file, "{{{\n" );
  report_t::print_text( file, sim );
  fprintf( file, "}}}\n" );
}

} // ANONYMOUS NAMESPACE =====================================================

// ===========================================================================
// Report
// ===========================================================================

void report_t::print_text( FILE* file, sim_t* sim )
{
  if( sim -> total_seconds == 0 ) return;

  int num_players = sim -> players_by_rank.size();

  fprintf( file, "\nDPS Ranking:\n" );
  fprintf( file, "%7.0f 100.0%%  Raid\n", sim -> raid_dps );
  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_rank[ i ];
    fprintf( file, "%7.0f  %4.1f%%  %s\n", p -> dps, 100 * p -> total_dmg / sim -> total_dmg, p -> name() );
  }

  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    fprintf( file, "\nPlayer=%s  DPS=%.1f (Error=+/-%.1f Range=+/-%.0f)", 
             p -> name(), p -> dps, p -> dps_error, ( p -> dps_max - p -> dps_min ) / 2.0 );

    if( p -> rps_loss > 0 )
    {
      fprintf( file, "  DPR=%.1f  RS=%.1f/%.1f  (%s)", 
               p -> dpr, p -> rps_loss, p -> rps_gain,
               util_t::resource_type_string( p -> primary_resource() ) );
    }

    fprintf( file, "\n" );

    print_core_stats   ( file, p );
    print_spell_stats  ( file, p );
    print_attack_stats ( file, p );
    print_defense_stats( file, p );
    print_actions      ( file, p );
  }

  print_gains        ( file, sim );
  print_procs        ( file, sim );
  print_uptime       ( file, sim );
  print_waiting      ( file, sim );
  print_performance  ( file, sim );
  print_scale_factors( file, sim );

  fprintf( file, "\n" );
}

// report_t::print_html ======================================================

void report_t::print_html( sim_t* sim )
{
  int num_players = sim -> players_by_name.size();

  if( num_players == 0 ) return;
  if( sim -> total_seconds == 0 ) return;
  if( sim -> html_file_str.empty() ) return;

  FILE* file = fopen( sim -> html_file_str.c_str(), "w" );
  if( ! file )
  {
    fprintf( stderr, "simcraft: Unable to open html file '%s'\n", sim -> html_file_str.c_str() );
    exit(0);
  }

  fprintf( file, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n" );
  fprintf( file, "<html>\n" );

  fprintf( file, "<head>\n" );
  fprintf( file, "<title>Simulationcraft Results</title>\n" );

  if( num_players > 1 ) print_html_menu_definition( file, sim );

  fprintf( file, "</head>\n" );
  fprintf( file, "<body>\n" );

  if( num_players > 1 ) print_html_raid( file, sim );

  print_html_scale_factors( file, sim );

  if( num_players > 1 ) print_html_menu_triggers( file, sim );

  fprintf( file, "<div id=\"players\">\n");
  
  for( int i=0; i < num_players; i++ )
  {
    print_html_player( file, sim -> players_by_name[ i ] );
  }

  fprintf( file, "</div>\n" );
 
  print_html_text( file, sim );

  fprintf( file, "</body>\n" );
  fprintf( file, "</html>" );

  fclose( file );
}

// report_t::print_wiki ======================================================

void report_t::print_wiki( sim_t* sim )
{
  int num_players = sim -> players_by_name.size();

  if( num_players == 0 ) return;
  if( sim -> total_seconds == 0 ) return;
  if( sim -> wiki_file_str.empty() ) return;

  FILE* file = fopen( sim -> wiki_file_str.c_str(), "w" );
  if( ! file )
  {
    fprintf( stderr, "simcraft: Unable to open wiki file '%s'\n", sim -> wiki_file_str.c_str() );
    exit(0);
  }

  if( num_players > 1 ) print_wiki_raid( file, sim );

  print_wiki_scale_factors( file, sim );

  for( int i=0; i < num_players; i++ )
  {
    print_wiki_player( file, sim -> players_by_name[ i ] );
  }

  print_wiki_text( file, sim );

  fclose( file );
}

// report_t::print_suite =====================================================

void report_t::print_suite( sim_t* sim )
{
  report_t::print_text( sim -> output_file, sim );
  report_t::print_html( sim );
  report_t::print_wiki( sim );
}

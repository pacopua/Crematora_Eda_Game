#include "Player.hh"
#include <queue>
#include <utility>

using namespace std;
#define infinito 0x7FFFFFFF
/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Pacopua

struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }
  int dj[8] = {0,+1,+1,+1,0,-1,-1,-1};
  int di[8] = {+1,+1,0,-1,-1,-1,0,+1}; 

  int busqueda_casilla(CellType u,  Pos p, int r, int k) {
    //cout << "busqueda_casilaa" << round() << endl;
    //busca el tipo de unidad en un radio de r

    int i_to_mat = p.i - r;
    int j_to_mat = p.j - r;
    //esto sirve para pasar de la matriz del mapa a la nueva matriz que quedaremos 
    vector<bool> casilla_encontrada(8, false);
    vector<int> casillas(8, infinito);
    
    for (int j = 0; j < 8; ++j) {


      vector<vector<int>> dist(r*2+1, vector<int>(r*2+1, -1));
      
      queue<pair<int,int>> Q;

      int x_real = di[j] + p.i;
      int y_real = dj[j] + p.j;
      int x_matriz = x_real - i_to_mat;
      int y_matriz = y_real - j_to_mat;
      
      if (y_real >= 80) y_real -= 80;
      else if (y_real < 0) y_real += 80;

      if (pos_ok(x_real, y_real, k)) {
        Cell c = cell(x_real, y_real, k);
      
      if(c.id == -1 and c.type == Cave and c.owner != me()) {casillas[j] = -1; casilla_encontrada[j] = true;}
      else if (c.type == Cave and c.id == -1) Q.push(pair<int,int>(p.i + di[j], p.j + dj[j]));
      
      while(not Q.empty()) {
        
        pair<int, int> pos = Q.front();
        Q.pop();


        for (int i = 0; i < 8; ++i) {

          x_real = di[i] + pos.first;
          y_real = dj[i] + pos.second;
          x_matriz = x_real - i_to_mat;
          y_matriz = y_real - j_to_mat;

          if (y_real >= 80) y_real -= 80;
          else if (y_real < 0) y_real += 80;

          if (pos_ok(x_real, y_real, k)) {
            Cell c = cell(x_real, y_real, k);
            if(x_matriz < (r*2+1) and x_matriz > -1 and y_matriz < (r*2+1) and y_matriz > -1 and dist[x_matriz][y_matriz] == -1) {
              if(c.type == Cave) {

                dist[x_matriz][y_matriz] = dist[pos.first-i_to_mat][pos.second-j_to_mat] + 1;

                if(dist[x_matriz][y_matriz] <= r and c.id == -1) Q.push(pair<int,int>(x_real, y_real));
                
                if(c.id == -1 and c.owner != me()) {

                  casillas[j] = dist[x_matriz][y_matriz];
                  casilla_encontrada[j] = true;
                  break; // salimos del for

                }
              }
            }
          }   
          if (casilla_encontrada[j]) break;
        }
      }
    }
    }
    int min_dist = infinito;
    int i_min_dist = -1;

    for(int i = 0; i < 8; ++i) { 
      if(min_dist > casillas[i] and casilla_encontrada[i]) {
        //cout << "canditado: " << i << endl;
        min_dist = casillas[i];
        i_min_dist = i;
      }
      else if (min_dist < infinito and min_dist == casillas[i]) {
        if(random(1,2) == 1) i_min_dist = i;
      }
    }
    return i_min_dist;

  }

  int busqueda_huida(UnitType u, Pos p, int r, int k, int id) {
    //busca el tipo de unidad en un radio de r

    //cout << "busqueda_huida" << round() << endl;

    int i_to_mat = p.i - r;
    int j_to_mat = p.j - r;
    //esto sirve para pasar de la matriz del mapa a la nueva matriz que quedaremos 

    vector<bool> casilla_encontrada(8, false);
    vector<int> casillas(8, -1);
    
    for (int j = 0; j < 8; ++j) {


      vector<vector<int>> dist(r*2+1, vector<int>(r*2+1, -1));
      
      queue<pair<int,int>> Q;
      int x_real = di[j] + p.i;
      int y_real = dj[j] + p.j;

      int x_matriz = x_real - i_to_mat;
      int y_matriz = y_real - j_to_mat;

      if (y_real >= 80) y_real -= 80;
      else if (y_real < 0) y_real += 80;
      
      int distancia;//distancia del objeto asignada para cada casilla

      if (pos_ok(x_real, y_real, k)) {
        Cell c = cell(x_real, y_real, k);
        if (c.type == Cave and c.id != -1 and unit(c.id).type == u and unit(c.id).player != me()) {
          casillas[j] = -1;
          casilla_encontrada[j] = true;
        }
        else if(c.type == Cave and c.id == -1) Q.push(pair<int,int>(p.i + di[j], p.j + dj[j]));

        while(not Q.empty()) {
        
        pair<int, int> pos = Q.front();
        Q.pop();
        
        for (int i = 0; i < 8; ++i) {

          x_real = di[i] + pos.first;
          y_real = dj[i] + pos.second;
          
          x_matriz = x_real - i_to_mat;
          y_matriz = y_real - j_to_mat;
          
          if (y_real >= 80) y_real -= 80;
          else if (y_real < 0) y_real += 80;

          if (pos_ok(x_real, y_real, k)) {
            //cout << "pos_ok" << endl;
            c = cell(x_real, y_real, k);
            if(x_matriz < (r*2+1) and x_matriz > -1 and y_matriz < (r*2+1) and y_matriz > -1 and dist[x_matriz][y_matriz] == -1) {
              if(c.type == Cave) {

                dist[x_matriz][y_matriz] = dist[pos.first-i_to_mat][pos.second-j_to_mat] + 1;

                if(dist[x_matriz][y_matriz] <= r and (c.id == -1 or c.id == id))Q.push(pair<int,int>(x_real, y_real));
                
                if(c.id != -1 and unit(c.id).type == u and unit(c.id).player != me()) {

                  casillas[j] = dist[x_matriz][y_matriz];
                  casilla_encontrada[j] = true;

                  break; // salimos del for
                }
              }
            }
          }   
        }
        if (casilla_encontrada[j]) break;
      }
      }
     
      }
    
    int max_dist = -2;
    int i_max_dist = -1;
    for(int i = 0; i < 8; ++i) { 
      if(max_dist < casillas[i] and casilla_encontrada[i]) {
        max_dist = casillas[i];
        i_max_dist = i;
      }
      else if (casilla_encontrada[i] and max_dist == casillas[i]) {
        if(random(1,2) == 1) i_max_dist = i;
      }
    }
    return i_max_dist;

  }

  /*
  Funcion para los Furyans, busca a otros Furyan y a pioneers, cada uno en un radio distinto
  */
  int busqueda_ataque(Pos p, int r, int r_pio, int r_fur, int k) {

    //busca el tipo de unidad en un radio de r
    int i_to_mat = p.i - r;
    int j_to_mat = p.j - r;
    //esto sirve para pasar de la matriz del mapa a la nueva matriz que quedaremos 

    vector<bool> casilla_encontrada(8, false);
    vector<int> casillas(8, infinito);
    
    for (int j = 0; j < 8; ++j) {


      vector<vector<int>> dist(r*2+1, vector<int>(r*2+1, -1));
      
      queue<pair<int,int>> Q;

      int x_real = di[j] + p.i;
      int y_real = dj[j] + p.j;
      int x_matriz = x_real - i_to_mat;
      int y_matriz = y_real - j_to_mat;
      
      if (y_real >= 80) y_real -= 80;
      else if (y_real < 0) y_real += 80;

      if (pos_ok(x_real, y_real, k)){
        
        Cell c = cell(x_real, y_real, k);
      
      if(c.id != -1 and (((unit(c.id).type == Pioneer and dist[x_matriz][y_matriz] <= r_pio)) or(unit(c.id).type == Furyan and dist[x_matriz][y_matriz] <= r_fur)) and unit(c.id).player != me() and c.type == Cave) {casilla_encontrada[j] = true; casillas[j] = -1;}
      
      else if (c.type == Cave) Q.push(pair<int,int>(p.i + di[j], p.j + dj[j]));
      
      while(not Q.empty()) {
        
        pair<int, int> pos = Q.front();
        Q.pop();


        for (int i = 0; i < 8; ++i) {

          x_real = di[i] + pos.first;
          y_real = dj[i] + pos.second;
          x_matriz = x_real - i_to_mat;
          y_matriz = y_real - j_to_mat;

          if (y_real >= 80) y_real -= 80;
          else if (y_real < 0) y_real += 80;

          if (pos_ok(x_real, y_real, k)) {
            //cout << "pos_ok" << endl;
            Cell c = cell(x_real, y_real, k);
            if(x_matriz < (r*2+1) and x_matriz > -1 and y_matriz < (r*2+1) and y_matriz > -1 and dist[x_matriz][y_matriz] == -1) {
              if(c.type == Cave) {

                dist[x_matriz][y_matriz] = dist[pos.first-i_to_mat][pos.second-j_to_mat] + 1;

                if(dist[x_matriz][y_matriz] <= r)Q.push(pair<int,int>(x_real, y_real));
                
                if(c.id != -1 and (((unit(c.id).type == Pioneer and dist[x_matriz][y_matriz] <= r_pio)) or(unit(c.id).type == Furyan and dist[x_matriz][y_matriz] <= r_fur)) and unit(c.id).player != me()) {

                  casillas[j] = dist[x_matriz][y_matriz];
                  casilla_encontrada[j] = true;

                  break; // salimos del for
                }
              }
            }
          }   
          if (casilla_encontrada[j]) break;
        }
      }
      
    }
    }
    int min_dist = infinito;
    int i_min_dist = -2;
    for(int i = 0; i < 8; ++i) { 
      if(min_dist > casillas[i] and casilla_encontrada[i]) {
        min_dist = casillas[i];
        i_min_dist = i;
      }
      else if (min_dist < infinito and min_dist == casillas[i]) {
        if(random(1,2) == 1) i_min_dist = i;
      }
    }
    return i_min_dist;

  }

  void play_pioneers() {
    vector<int> id_pio = pioneers(me());
    int n = id_pio.size();
    for (int i = 0; i < n; ++i) {
      Unit p = unit(id_pio[i]);
      int dir;
      if((dir = busqueda_huida(Hellhound, p.pos, 5, 0, id_pio[i])) > -1) command(id_pio[i], Dir(dir));
      else if((dir = busqueda_huida(Furyan, p.pos, 4, 0, id_pio[i])) > -1) command(id_pio[i], Dir(dir));
      else if((dir = busqueda_casilla(Cave, p.pos, 40, 0)) > -1) command(id_pio[i], Dir(dir));
      else {
        dir = random(0,7);
        while (not pos_ok(p.pos.i + di[dir], p.pos.i + di[dir], p.pos.k) and cell(p.pos.i + di[dir], p.pos.i + di[dir], p.pos.k).type == Rock) dir = random(0,7);
        command(id_pio[i], Dir(dir));
      }      
    }
  }

  void play_furyans() {
    vector<int> id_fury = furyans(me());
    int n = id_fury.size();
    for (int i = 0; i < n; ++i) {
      Unit p = unit(id_fury[i]);
      int dir;
      if((dir = busqueda_huida(Hellhound, p.pos, 5, 0, id_fury[i])) > -1) command(id_fury[i], Dir(dir));
      else if((dir = busqueda_ataque(p.pos, 20, 20, 8, 0)) > -1) command(id_fury[i], Dir(dir));
      else {
        dir = random(0,7);
        while (not pos_ok(p.pos.i + di[dir], p.pos.i + di[dir], p.pos.k) and cell(p.pos.i + di[dir], p.pos.i + di[dir], p.pos.k).type == Rock) dir = random(0,7);
        command(id_fury[i], Dir(dir));
      }      
    }
  }


  /**
   * Types and attributes for your player can be defined here.
   */

  /**
   * Play method, invoked once per each round.
   */
  virtual void play () {
    play_furyans();
    play_pioneers();
  }

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

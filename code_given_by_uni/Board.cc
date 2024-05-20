#include "Board.hh"
#include "Action.hh"


void Board::capture (int id, int pl) {
  Unit& u = unit_[id];
  if (pl >= 0) assert(u.player != pl);
  else if (u.player == -1) assert(u.type == Necromonger);

  grid_[u.pos.i][u.pos.j][u.pos.k].id = -1;
  u.health = 0;

  if (u.type == Necromonger) u.pos = Pos(-1, -1, -1);
  else {
    if (pl == -1) {
      int r = random(0, nb_players_ - 2);
      pl = r + (r >= u.player); // the new pl must be different
    }
    u.player = pl;
  }
}


void Board::step (int id, Pos p2) {
  Unit& u = unit_[id];
  Pos p1 = u.pos;
  assert(p1 != p2);
  Cell& c1 = grid_[p1.i][p1.j][p1.k];
  Cell& c2 = grid_[p2.i][p2.j][p2.k];
  assert(c2.id == -1);
  if (c1.type != Elevator) assert(p1.k == p2.k);
  c1.id = -1;
  c2.id = id;
  int pl = unit_[id].player;
  if (u.type == Pioneer) {
    if (c2.type == Cave) c2.owner = pl;
    if (c2.gem) {
      c2.gem = false;
      ++nb_gems_[pl];
    }
  }
  u.pos = p2;
}


inline int Board::max_distance (Pos p1, Pos p2) {
  int v = abs(p1.i - p2.i);
  int h = (p1.j < p2.j ?
           min(p2.j - p1.j, 80 + p1.j - p2.j) : // this should be done better
           min(p1.j - p2.j, 80 + p2.j - p1.j));
  return max(v, h);
}


inline int Board::manhattan (Pos p1, Pos p2) {
  int v = abs(p1.i - p2.i);
  int h = (p1.j < p2.j ?
           min(p2.j - p1.j, 80 + p1.j - p2.j) : // this should be done better
           min(p1.j - p2.j, 80 + p2.j - p1.j));
  return v + h;
}


inline int Board::distance_hellhounds (Pos p) {
  int res = 1e8;
  for (int hid : hellhounds_) res = min(res, max_distance(p, unit_[hid].pos));
  return res;
}


inline bool Board::adjacent_hellhound (Pos p) {
  return p.k == 0 and distance_hellhounds(p) <= 1;
}


// id is a valid unit id, moved by its player, and dir is valid.
bool Board::move (int id, Dir dir) {
  Unit& u = unit_[id];
  UnitType ut = u.type;
  int pl1 = u.player;
  if (ut == Furyan or ut == Pioneer) assert(pl1 != -1);
  else assert(pl1 == -1);

  Pos p1 = u.pos;
  assert(pos_ok(p1));

  if (dir == None) return true;

  Cell& c1 = grid_[p1.i][p1.j][p1.k];
  CellType ct1 = c1.type;
  Pos p2 = p1 + dir;
  if (not pos_ok(p2)) return false;
  if ((dir == Up or dir == Down) and ct1 != Elevator) return false;

  Cell& c2 = grid_[p2.i][p2.j][p2.k];
  int id2 = c2.id;
  CellType ct2 = c2.type;
  assert(ct1 != Rock);

  if (ut == Hellhound) {
    assert(pl1 == -1 and p1.k + p2.k == 0 and id2 == -1);
    step(id, p2);
    for (int d = 0; d < 8; ++d) {
      Pos p3 = p2 + Dir(d);
      if (pos_ok(p3)) {
        Cell& c = grid_[p3.i][p3.j][p3.k];
        if (c.id != -1) capture(c.id, -1);
      }
    }
    return true;
  }

  if (ut == Necromonger) assert(p1.k + p2.k == 2 and u.turns == 0);

  if (id2 == -1) {
    if (ct2 == Rock) return false;
    if (adjacent_hellhound(p2) or daylight(p2)) capture(id, -1);
    else step(id, p2);
    return true;
  }

  Unit& u2 = unit_[id2];
  if (pl1 == -1) assert(u2.player != -1);
  else if (pl1 == u2.player) return false;

  if (u2.type == Hellhound) { // suicide going down an elevator
    capture(id, -1);
    return true;
  }

  if (ut == Pioneer) return false;

  int mn, mx;
  if (ut == Furyan) {
    mn = min_damage_furyans();
    mx = max_damage_furyans();
  }
  else {
    mn = min_damage_necromongers();
    mx = max_damage_necromongers();
  }

  u2.health -= min(u2.health, random(mn, mx));
  if (u2.health <= 0) capture(id2, pl1);
  return true;
}


void Board::compute_nb_cells () {
  nb_cells_ = vector<int>(nb_players_, 0);
  for (int i = 0; i < rows_; ++i)
    for (int j = 0; j < cols_; ++j) {
      int pl = grid_[i][j][0].owner;
      if (pl != -1) {
        assert(grid_[i][j][0].type == Cave);
        ++nb_cells_[pl];
      }
    }
}


// ***************************************************************************


void Board::new_unit (int id, int pl, Pos pos, UnitType t) {
  int h;
  if (t == Furyan) h = furyans_health();
  else if (t == Pioneer) h = pioneers_health();
  else if (t == Necromonger) h = 0;
  else if (t == Hellhound) h = -1;
  else assert(false);

  if (t == Furyan or t == Pioneer) assert(pl >= 0 and pl < nb_players());
  else assert(pl == -1);

  unit_[id] = Unit(t, id, pl, h, -1, pos);

  if (t == Necromonger) return;

  assert(pos_ok(pos) and pos.k == 0 and grid_[pos.i][pos.j][pos.k].id == -1);
  CellType ct = grid_[pos.i][pos.j][pos.k].type;
  assert(ct == Cave);

  grid_[pos.i][pos.j][pos.k].id = id;
  if (t == Hellhound) return;

  assert(not adjacent_hellhound(pos));
  if (t == Pioneer) grid_[pos.i][pos.j][pos.k].owner = pl;
}


void Board::ban (Pos pos, int dist, set<Pos>& banned) {
  for (int x = -dist; x <= dist; ++x)
    for (int y = -dist; y <= dist; ++y) banned.insert(pos + Pos(x, y, 0));
}


void Board::generate_units () {
  int r = rows();
  int c = cols();
  int id = unit_.size();
  set<Pos> banned;

  for (int rep = 0; rep < nb_hellhounds(); ++rep) {
    int x, y;
    do {
      x = random(0, r - 1);
      y = random(0, c - 1);
    } while (grid_[x][y][0].type != Cave
             or banned.find(Pos(x, y, 0)) != banned.end());
    new_unit(--id, -1, Pos(x, y, 0), Hellhound);
    ban(Pos(x, y, 0), 3, banned);
  }

  vector<Pos> V;
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j)
      if (grid_[i][j][0].type == Cave
          and banned.find(Pos(i, j, 0)) != banned.end())
        V.push_back(Pos(i, j, 0));
  int m = V.size();
  vector<int> perm1 = random_permutation(m);

  int tot_player = nb_furyans() + nb_pioneers();
  int n = nb_players()*tot_player;
  vector<int> perm2 = random_permutation(n);

  set<Pos> copy = banned;
  int j = -1;
  int i = 0;
  while (j < m and i < n) {
    Pos pos;
    while (++j < m and banned.find(pos = V[perm1[j]]) != banned.end()) ;
    if (j < m) {
      int id2 = perm2[i++];
      int pl = id2/tot_player;
      bool pioneer = (id2%tot_player >= nb_furyans());
      new_unit(id2, pl, pos, (pioneer ? Pioneer : Furyan));
      ban(pos, 1, banned);
      copy.insert(pos); // this line should also be added to the Moria game...
    }
  }

  if (i < n) { // only for nasty initial configurations
    V.clear();
    for (int i = 0; i < r; ++i)
      for (int j = 0; j < c; ++j)
        if (grid_[i][j][0].type == Cave and grid_[i][j][0].id == -1
            and not adjacent_hellhound(Pos(i, j, 0))
            and copy.find(Pos(i, j, 0)) == copy.end())
          V.push_back(Pos(i, j, 0));
    m = V.size();
    assert(n - i <= m);

    perm1 = random_permutation(m);
    j = -1;
    while (i < n) {
      Pos pos = V[perm1[++j]];
      int id2 = perm2[i++];
      int pl = id2/tot_player;
      bool pioneer = (id2%tot_player >= nb_furyans());
      new_unit(id2, pl, pos, (pioneer ? Pioneer : Furyan));
    }
  }

  for (int id = n; id < n + max_nb_necromongers(); ++id)
    new_unit(id, -1, Pos(-1, -1, -1), Necromonger);
}


// ***************************************************************************


Board::Board (istream& is, int seed) {
  set_random_seed(seed);
  *static_cast<Settings*>(this) = Settings::read_settings(is);
  int np = nb_players();
  names_ = vector<string>(np);
  read_generator_and_grid(is);
  round_ = 0;
  nb_cells_ = nb_gems_ = vector<int>(np, 0);
  cpu_status_ = vector<double>(np, 0);
  unit_ = vector<Unit>(np*(nb_furyans() + nb_pioneers())
                       + max_nb_necromongers() + nb_hellhounds());
  generate_units();
  update_vectors_by_player();
  compute_nb_cells();
}


void Board::print_preamble (ostream& os) const {
  os << version() << endl;
  os << "nb_players          " << nb_players() << endl;
  os << "nb_rounds           " << nb_rounds() << endl;
  os << "nb_furyans          " << nb_furyans() << endl;
  os << "nb_pioneers         " << nb_pioneers() << endl;
  os << "max_nb_necromongers " << max_nb_necromongers() << endl;
  os << "nb_hellhounds       " << nb_hellhounds() << endl;
  os << "nb_elevators        " << nb_elevators() << endl;
  os << "gem_value           " << gem_value() << endl;
  os << "turns_to_land       " << turns_to_land() << endl;
  os << "rows                " << rows() << endl;
  os << "cols                " << cols() << endl;
}


void Board::print_names (ostream& os) const {
  os << "names              ";
  for (string s : names_) os << ' ' << s;
  os << endl;
}


void Board::print_state (ostream& os) const {
  os << endl << endl;

  for (int i = 0; i < rows_; ++i) {
    for (int j = 0; j < cols_; ++j) {
      const Cell& c = grid_[i][j][0];
      CellType ct = c.type;
      if (ct == Rock) os << 'R';
      else if (ct == Elevator) os << 'E';
      else {
        assert(ct == Cave);
        if (c.owner != -1) os << c.owner;
        else os << 'C';
      }
    }
    os << endl;
  }

  for (int i = 0; i < rows_; ++i) {
    for (int j = 0; j < cols_; ++j) {
      const Cell& c = grid_[i][j][1];
      CellType ct = c.type;
      if (ct == Elevator) os << 'E';
      else {
        assert(ct == Outside);
        if (c.gem) os << 'G';
        else os << 'O';
      }
    }
    os << endl;
  }

  os << endl;
  os << "round " << round_ << endl;

  os << "nb_cells";
  for (int nc : nb_cells_) os << " " << nc;
  os << endl;

  os << "nb_gems";
  for (int ts : nb_gems_) os << " " << ts;
  os << endl;

  os << "status";
  for (double st : cpu_status_) os << " " << st;
  os << endl;

  for (Unit un : unit_) os << un << endl;
  os << endl;
}


void Board::print_results () const {
  int max_score = 0;
  vector<int> v;
  for (int pl = 0; pl < nb_players(); ++pl) {
    int sc = nb_cells(pl) + gem_value()*nb_gems(pl);
    cerr << "info: player " << name(pl) << " got score " << sc << endl;
    if (sc > max_score) {
      max_score = sc;
      v = vector<int>(1, pl);
    }
    else if (sc == max_score) v.push_back(pl);
  }

  cerr << "info: player(s)";
  for (int pl : v) cerr << " " << name(pl);
  cerr << " got top score" << endl;
}


// ***************************************************************************


void Board::place (int id, Pos p) {
  unit_[id].pos = p;
  Cell& c = grid_[p.i][p.j][p.k];
  UnitType ut = unit_[id].type;
  if (ut != Necromonger) c.id = id; // necros are still descending
  if (ut == Pioneer and c.type == Cave) c.owner = unit_[id].player;
}


void Board::move_sun () {
  int m = cols_>>1;
  int t1 = (round()<<1)%cols_;
  for (int i = 0; i < rows_; ++i) {
    grid_[i][t1][1].gem = grid_[i][t1+1][1].gem = false;
    int id1 = grid_[i][t1][1].id;
    if (id1 != -1) capture(id1, -1);
    int id2 = grid_[i][t1+1][1].id;
    if (id2 != -1) capture(id2, -1);
  }
}


void Board::create_gems () {
  if (random(1, inv_prob_gem()) == 1) {
    int m = cols_>>1;
    int t1 = (round()<<1)%cols_;
    int t2 = t1 + m;
    if (t2 >= cols_) t2 -= cols_;

    vector<Pos> V;
    for (int i = 0; i < rows_; ++i) {
      if (grid_[i][t2][1].type == Outside) V.push_back(Pos(i, t2, 1));
      if (grid_[i][t2+1][1].type == Outside) V.push_back(Pos(i, t2+1, 1));
    }

    int n = V.size();
    if (n > 0) {
      Pos p = V[random(0, n - 1)];
      grid_[p.i][p.j][p.k].gem = true;
    }
  }
}


void Board::spawn_necromongers (const vector<int>& dead_necromongers) {
  int num = dead_necromongers.size();
  if (num == 0) return;

  if ((int)necromongers_.size() < max_nb_necromongers() and random(1, inv_prob_necromonger()) == 1) {
    int m = cols_>>1;
    int t1 = (round()<<1)%cols_;
    int t2 = t1 + m;
    if (t2 >= cols_) t2 -= cols_;

    vector<Pos> V;
    for (int i = 0; i < rows_; ++i) {
      if (grid_[i][t2][1].type == Outside and not grid_[i][t2][1].gem)
        V.push_back(Pos(i, t2, 1));
      if (grid_[i][t2+1][1].type == Outside and not grid_[i][t2+1][1].gem)
        V.push_back(Pos(i, t2+1, 1));
    }

    int n = V.size();
    if (n > 0) {
      int id = dead_necromongers[random(0, num - 1)];
      unit_[id].health = necromongers_health();
      unit_[id].turns = turns_to_land();
      place(id, V[random(0, n - 1)]);
    }
  }
}


void Board::spawn_units (const vector<int>& dead_others) {
  int r = rows();
  int c = cols();
  set<Pos> banned;
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j) {
      int id = grid_[i][j][0].id;
      if (id != -1) {
        int dist = (unit_[id].type == Hellhound ? 3 : 1);
        ban(Pos(i, j, 0), dist, banned);
      }
    }

  vector<Pos> V;
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j)
      if (grid_[i][j][0].type == Cave and grid_[i][j][0].id == -1
          and banned.find(Pos(i, j, 0)) == banned.end())
        V.push_back(Pos(i, j, 0));

  int n = dead_others.size();
  int m = V.size();
  vector<int> perm1 = random_permutation(m);
  vector<int> perm2 = random_permutation(n);

  int j = -1;
  int i = 0;
  while (j < m and i < n) {
    Pos pos;
    while (++j < m and banned.find(pos = V[perm1[j]]) != banned.end()) ;
    assert(j < m);
    int id = dead_others[perm2[i++]];
    unit_[id].health = (unit_[id].type == Pioneer ?
                        pioneers_health() : furyans_health());
    place(id, pos);
    ban(pos, 1, banned);
  }
}


void Board::move_necromongers () {
  int r = rows_;
  int c = cols_;
  vector<vector<int>> T(r, vector<int>(c, 1e9));
  queue<Pos> Q;
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j) {
      const Cell& c = grid_[i][j][1];
      if (c.id != -1 and unit_[c.id].player != -1) {
        T[i][j] = 0;
        Q.push(Pos(i, j, 1));
      }
    }

  while (not Q.empty()) {
    Pos p1 = Q.front(); Q.pop();
    for (int d = 0; d < 8; ++d) {
      Pos p2 = p1 + Dir(d);
      if (pos_ok(p2) and T[p2.i][p2.j] == 1e9) {
        T[p2.i][p2.j] = T[p1.i][p1.j] + 1;
        Q.push(p2);
      }
    }
  }

  for (int id_necromonger : necromongers_) {
    const Unit& u = unit_[id_necromonger];
    if (unit_[id_necromonger].health > 0 and u.turns == 0) {
      Pos p1 = u.pos;
      vector<int> A, D;
      int minim = 1e8;
      for (int d = 0; d < 8; ++d) {
        Pos p2 = p1 + Dir(d);
        if (pos_ok(p2)) {
          int id = grid_[p2.i][p2.j][p2.k].id;
          if (id == -1) {
            int dist = T[p2.i][p2.j];
            if (dist < minim) {
              minim = dist;
              D = vector<int>(1, d);
            }
            else if (dist == minim) D.push_back(d);
          }
          else if (unit_[id].player != -1) A.push_back(d);
        }
      }

      Dir dir = None;
      if (not A.empty()) dir = Dir(A[random(0, A.size() - 1)]);
      else if (not D.empty()) dir = Dir(D[random(0, D.size() - 1)]);
      else {
        vector<int> V;
        for (int d = 1; d <= 3; ++d) {
          Pos p2 = p1 + Dir(d);
          if (pos_ok(p2) and grid_[p2.i][p2.j][p2.k].id == -1) V.push_back(d);
        }
        if (not V.empty()) dir = Dir(V[random(0, V.size() - 1)]);
      }

      assert(move(id_necromonger, dir));
      actions_done_.push_back(Movement(id_necromonger, dir));
    }
  }

  for (int id_necromonger : necromongers_) {
    Unit& u = unit_[id_necromonger];
    if (u.turns > 0) {
      Pos p = u.pos;
      if (--u.turns == 0) {
        int id2 = grid_[p.i][p.j][1].id;
        if (id2 != -1) capture(id2, -1);
        grid_[p.i][p.j][1].id = id_necromonger;
      }
    }
  }
}


void Board::move_hellhounds () {
  int r = rows_;
  int c = cols_;
  vector<vector<int>> T(r, vector<int>(c, 1e9));
  queue<Pos> Q;
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j) {
      const Cell& c = grid_[i][j][0];
      CellType ct = c.type;
      if (ct != Rock) {
        if (c.id != -1 and unit_[c.id].player != -1) {
          T[i][j] = 0;
          Q.push(Pos(i, j, 0));
        }
        else T[i][j] = 1e7;
      }
    }

  while (not Q.empty()) {
    Pos p1 = Q.front(); Q.pop();
    for (int d = 0; d < 8; ++d) {
      Pos p2 = p1 + Dir(d);
      if (pos_ok(p2) and T[p2.i][p2.j] == 1e7) {
        T[p2.i][p2.j] = T[p1.i][p1.j] + 1;
        Q.push(p2);
      }
    }
  }

  for (int hid : hellhounds_) {
    const Unit& u = unit_[hid];
    Pos p1 = u.pos;
    vector<int> D;
    int minim = 1e8;
    for (int d = 0; d < 8; ++d) {
      Pos p2 = p1 + Dir(d);
      if (pos_ok(p2)) {
        bool ok = true;
        for (int hid2 : hellhounds_)
          if (hid != hid2 and max_distance(p2, unit_[hid2].pos) <= 1)
            ok = false;
        if (ok) {
          int dist = T[p2.i][p2.j];
          if (dist < minim) {
            minim = dist;
            D = vector<int>(1, d);
          }
          else if (dist == minim) D.push_back(d);
        }
      }
    }

    Dir dir = None;
    if (not D.empty()) dir = Dir(D[random(0, D.size() - 1)]);

    assert(move(hid, dir));
    actions_done_.push_back(Movement(hid, dir));
  }
}


void Board::heal_units () {
  int h = health_recovery();
  for (Unit& u : unit_)
    if (u.health > 0) {
      if (u.type == Pioneer) u.health = min(u.health + h, pioneers_health());
      else if (u.type == Furyan)
        u.health = min(u.health + h, furyans_health());
      else u.health = min(u.health + h, necromongers_health());
    }
}


void Board::next (const vector<Action>& act, ostream& os) {
  int np = nb_players();
  int nu = nb_units();

  // chooses (at most) one movement per unit
  vector<int> seen(nu, false);
  vector<vector<Movement>> v(np);
  for (int pl = 0; pl < np; ++pl)
    for (const Movement& m : act[pl].v_) {
      int id = m.id;
      Dir dir = m.dir;
      if (not unit_ok(id))
        cerr << "warning: id out of range or dead unit:" << id << endl;
      else {
        const Unit& u = unit(id);
        if (u.player != pl)
          cerr << "warning: not own unit: " << id << ' ' << u.player << ' '
               << pl << endl;
        else {
          _my_assert(not seen[id],
                     "More than one command for the same unit.");
          seen[id] = true;
          if (not dir_ok(dir))
            cerr << "warning: direction not valid: " << dir << endl;
          else if (dir != None) v[pl].push_back(Movement(id, dir));
        }
      }
    }

  // Makes all players' movements using a random order,
  // but respecting the relative order of the units of the same player.
  // Permutations are not equally likely to avoid favoring leading groups.
  // This is not exactly true in the Moria game...
  actions_done_.clear();

  int num = 0;
  for (int pl = 0; pl < np; ++pl) num += v[pl].size();

  vector<int> index(np, 0);
  while (num > 0) {
    vector<int> V;
    for (int pl = 0; pl < np; ++pl)
      if (index[pl] < (int)v[pl].size()) V.push_back(pl);
    int q = V.size();
    assert(q > 0);

    vector<int> perm = random_permutation(q);
    for (int i = 0; i < q; ++i) {
      int pl = V[perm[i]];
      Movement m = v[pl][index[pl]++];
      if (unit_[m.id].health > 0 and move(m.id, m.dir))
        actions_done_.push_back(m);
    }
    num -= q;
  }

  move_necromongers();

  move_hellhounds();

  os << "movements" << endl;
  Action::print_actions(actions_done_, os);

  move_sun();

  vector<int> dead_necros, dead_others;
  for (const Unit& u : unit_)
    if (u.health == 0)
      (u.type == Necromonger ? dead_necros : dead_others).push_back(u.id);

  create_gems();

  spawn_necromongers(dead_necros);

  spawn_units(dead_others);

  heal_units();

  update_vectors_by_player();

  compute_nb_cells();

  ++round_;
}


// ***************************************************************************


void Board::read_generator_and_grid (istream& is) {
  is >> generator_;
  if (generator_ == "FIXED") read_grid(is);
  else {
    vector<int> param;
    int x;
    while (is >> x) param.push_back(x);
    if (generator_ == "GENERATOR") generator(param);
    else _my_assert(false, "Unknow grid generator.");
  }
}


bool Board::path (Pos p1, Pos p2) {
  int r = rows();
  int c = cols();
  int x = p1.i;
  int y = p1.j;
  M_[x][y] = 'C';
  int q = 0; // to avoid an infinite loop
  while ((x != p2.i or y != p2.j) and ++q < 1000) {
    int mx_distance = 0;
    for (int d = 0; d < 8; d += 2) {
      Pos p3 = Pos(x, y, 0) + Dir(d);
      if (p3.i >= 1 and p3.i < r - 1)
        mx_distance = max(mx_distance, max_distance(p2, p3));
    }

    vector<Pos> V;
    vector<int> weight;
    int sum = 0;
    for (int d = 0; d < 8; ++d) {
      Pos p3 = Pos(x, y, 0) + Dir(d);
      if (p3.i >= 1 and p3.i < r - 1) {
        V.push_back(p3);
        int w = mx_distance - max_distance(p2, p3) + 1;
        weight.push_back(w);
        sum += w;
      }
    }
    if (sum == 0) return false;

    int ran = random(1, sum);
    int i = -1;
    int acum = 0;
    while (acum < ran) acum += weight[++i];
    x = V[i].i;
    y = V[i].j;
    M_[x][y] = 'C';
  }

  return q < 1000;
}


bool Board::generate_paths () {
  int r = rows();
  int c = cols();
  M_ = vector<vector<char>>(r, vector<char>(c, 'R'));

  vector<Pos> V;
  for (int i = 2; i < r - 2; ++i)
    for (int j = 0; j < c; ++j) V.push_back(Pos(i, j, 0));

  // find nb_elevators() positions not too close to each other
  int n = V.size();
  vector<int> perm = random_permutation(n);
  vector<Pos> E;
  for (int a = 0; a < n and (int)E.size() < nb_elevators(); ++a) {
    int b = perm[a];
    bool ok = true;
    for (Pos p : E)
      if (manhattan(p, V[b]) < 10) {
        ok = false;
        break;
      }
    if (ok) E.push_back(V[b]);
  }

  if ((int)E.size() < nb_elevators()) return false;

  // compute all the distances
  vector<vector<int>> D(nb_elevators(), vector<int>(nb_elevators()));
  for (int a = 0; a < nb_elevators(); ++a)
    for (int b = a + 1; b < nb_elevators(); ++b)
      D[a][b] = max_distance(E[a], E[b]);

  // inverse kruskal algorithm
  vector<int> repre(nb_elevators());
  for (int i = 0; i < nb_elevators(); ++i) repre[i] = i;
  int q = nb_elevators();
  while (q > 1) {
    int a_max = -33;
    int b_max = -33;
    int dist_max = 0;
    for (int a = 0; a < nb_elevators(); ++a)
      for (int b = a + 1; b < nb_elevators(); ++b)
        if (D[a][b] and D[a][b] > dist_max) {
          a_max = a;
          b_max = b;
          dist_max = D[a][b];
        }

    D[a_max][b_max] = 0;
    if (repre[a_max] != repre[b_max]) {
      int r = repre[b_max];
      for (int i = 0; i < nb_elevators(); ++i)
        if (repre[i] == r) repre[i] = repre[a_max];
      --q;
      if (not path(E[a_max], E[b_max])) return false;
    }
  }

  for (Pos p : E)
    for (int a = -1; a <= 1; ++a)
      for (int b = -1; b <= 1; ++b) {
        Pos p2 = p + Pos(a, b, 0);
        M_[p2.i][p2.j] = (a or b ? 'C' : 'E');
      }
  return true;
}


void Board::generate_grid () {
  int r = rows();
  int c = cols();

  while (not generate_paths()) cerr << "Bad grid. Trying again.." << endl;

  int cave = 0;
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j)
      if (M_[i][j] == 'C') ++cave;

  if (cave < 1200 or cave > 1500) {
    cerr << "Wrong number of cave cells: "
         << cave << ". Generating another grid.." << endl;
    generate_grid();
    return;
  }
}


void Board::generator (vector<int> param) {
  int num = param.size();
  _my_assert(num == 0, "GENERATOR requires no parameters.");

  int r = rows();
  int c = cols();
  _my_assert(r == 40 and c == 80, "GENERATOR with unexpected sizes.");

  generate_grid();

  grid_ = vector<vector<vector<Cell>>>(r, vector<vector<Cell>>(c, vector<Cell>(2)));
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j) grid_[i][j][0] = char2cell(M_[i][j]);
  for (int i = 0; i < r; ++i)
    for (int j = 0; j < c; ++j)
      grid_[i][j][1] = (M_[i][j] == 'E' ? char2cell('E') : char2cell('O'));
}

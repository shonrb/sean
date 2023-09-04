/* Monty Hall problem proof by simulation */
#include <random>
#include <cstdio>

void do_simulation(int play_count, bool swap)
{
  auto dev = std::random_device{};
  auto rng = std::mt19937(dev());
  auto win_count = 0;

  auto get_door = std::uniform_int_distribution<>(0, 2);

  for (int _ = 0; _ < play_count; ++_) {
    auto winning = get_door(rng);
    auto chosen  = get_door(rng);
    int revealed;
    while (true) {
      revealed = get_door(rng);
      if (revealed != winning && revealed != chosen) {
        break;
      }
    }
    if (swap) {
      chosen = 3 - (chosen + revealed);
    }
    win_count += chosen == winning;
  }
  auto win_rate = static_cast<float>(win_count) / play_count;
  printf(
    "While %s: won %d out of %d times, or a %f win rate\n",
    swap ? "swapping" : "not swapping",
    win_count,
    play_count,
    win_rate
  );
}

int main() 
{
  int test_count = 100000;
  do_simulation(test_count, false);
  do_simulation(test_count, true);
}

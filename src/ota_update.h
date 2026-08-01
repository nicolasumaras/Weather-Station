#pragma once

class StationOta {
 public:
  void begin();
  void loop(bool stationOnline);

 private:
  bool started_ = false;
};

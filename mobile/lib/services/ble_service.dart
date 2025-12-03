
// na razie symulacja

class BleService{
  Future<List> scanDevices() async {
    await Future.delayed(Duration(seconds: 2));
    return ["Doniczka-Marcin","Doniczka-Eliza", "Doniczka_Kuba"];
  }
}
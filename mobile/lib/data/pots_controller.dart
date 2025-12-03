import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

class PotsController extends ChangeNotifier {
  List<dynamic> pots =[];
  bool isLoading = false;
  Object ?error = null;

  PotsController(){
    fetchPots();
    print(pots.length);
  }

  Future<void> fetchPots() async{
    if(pots.isNotEmpty) return;
    isLoading = true;
    notifyListeners();
    try {
      final jsonString = await rootBundle.loadString("assets/json/pots.json");
      pots = json.decode(jsonString);
    }catch (e){
      error = e;
      pots = [];
    }finally{
      isLoading = false;
      notifyListeners();
    }
  }
}

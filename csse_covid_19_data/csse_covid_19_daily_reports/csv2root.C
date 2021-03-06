#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TDatime.h"

#include <fstream>
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

TObjArray *GetColumns(const TString &str)
{
   TPRegexp r("\"([\\w\\s,().]+)\",?");

   TObjArray *colL = new TObjArray();
   colL->SetOwner();
   Int_t start = 0;
   while (1) {
     TString subStr = str(r,start);
     const TString stripStr = subStr.Strip(TString::kTrailing,',');
     colL->Add(new TObjString(stripStr));
     const Int_t l = subStr.Length();
     if (l<=0) break;
     start += l;
   }

   return colL;
}

void csv2root(TString filenames) {
   TFile f("data_covid19.root","RECREATE");
   TTree tr("tree","tree");
   // tr.ReadFile(filename, "Province/C:Country/C:LastUpdate/C:Confirmed/I:Deaths/I:Recovered/I",',');

   // char Province[99];
   // char Country[99];
   // char LastUpdated[99];
   // int Cases(0), Deaths(0), Recovered(0), DeltaCases(0), DeltaDeaths(0), DeltaRecovered(0);
   vector<TString> Province, Country, LastUpdated;
   vector<int> Cases, Deaths, Recovered, DeltaCases, DeltaDeaths, DeltaRecovered;
   int totCases(0),totDeaths(0),totRecovered(0);
   TDatime date;

   map<TString,int> prevCases;
   map<TString,int> prevDeaths;
   map<TString,int> prevRecovered;

   tr.Branch("Province",&Province);
   tr.Branch("Country",&Country);
   tr.Branch("LastUpdated",&LastUpdated);
   tr.Branch("Cases",&Cases);
   tr.Branch("Deaths",&Deaths);
   tr.Branch("Recovered",&Recovered);
   tr.Branch("DeltaCases",&DeltaCases);
   tr.Branch("DeltaDeaths",&DeltaDeaths);
   tr.Branch("DeltaRecovered",&DeltaRecovered);
   tr.Branch("date",&date);

   TString tok;
   Ssiz_t from = 0;
   while (filenames.Tokenize(tok, from, " ")) {
      ifstream ifs(tok.Data());

      // parse file name, make it a date
      TString parsedate(tok);
      parsedate.ReplaceAll(".csv","");
      TString tokdate;
      Ssiz_t fromdate = 0;
      int day,month,year;
      int cntdate(0);
      while (parsedate.Tokenize(tokdate, fromdate, "-")) {
         if (cntdate==0) month = atoi(tokdate.Data());
         else if (cntdate==1) day = atoi(tokdate.Data());
         else if (cntdate==2) year = atoi(tokdate.Data());

         cntdate++;
      }
      date.Set(year,month,day,0,0,0);

      char theline[1024];
      while(ifs.is_open()) {
         ifs.getline(theline,1024);
         TString thelineT(theline);
         if (thelineT=="") break;
         // cout << theline << "EOL" << endl;

         // the line may need some fixing
         TObjArray *col = GetColumns(thelineT);
         for (Int_t i = 0; i < col->GetLast()+1; i++) {
            TString tmps = ((TObjString *)col->At(i))->GetString();
            if (tmps != "") {
               TString tmpsnew = tmps;
               tmpsnew.ReplaceAll(",",";");
               thelineT.ReplaceAll(tmps,tmpsnew);
               thelineT.ReplaceAll("\"","");
            }
         }

         TString tok2;
         Ssiz_t from2 = 0;
         int cnt=0;
         while (thelineT.Tokenize(tok2, from2, ",")) {
            // fix for China
            if (cnt==0) Province.push_back(tok2);
            else if (cnt==1) {
               Country.push_back(tok2);
               // workaround for France: if Province is empty, then copy Country into it
               if (Province.back()=="") Province.back() = Country.back();
               // workaround for China
               if (Country.back()=="Mainland China") Country.back() = "China";
               // workaround for S. Korea
               if (Country.back().Contains("Korea")) Country.back() = "South Korea";
               // workaround for UK
               if (Country.back().Contains("Iran")) Country.back() = "Iran";
               // workaround for Iran
               if (Country.back().Contains("United Kingdom")) Country.back() = "UK";
            } else if (cnt==2) LastUpdated.push_back(tok2);
            else if (cnt==3) {
               int CasesToday = atoi(tok2);
               totCases += CasesToday;
               DeltaCases.push_back(CasesToday - prevCases[Province.back()+Country.back()]);
               Cases.push_back(CasesToday);
               prevCases[Province.back()+Country.back()] = CasesToday;
            } else if (cnt==4) {
               int DeathsToday = atoi(tok2);
               totDeaths += DeathsToday;
               DeltaDeaths.push_back(DeathsToday - prevDeaths[Province.back()+Country.back()]);
               Deaths.push_back(DeathsToday);
               prevDeaths[Province.back()+Country.back()] = DeathsToday;
            } else if (cnt==5) {
               int RecoveredToday = atoi(tok2);
               totRecovered += RecoveredToday;
               DeltaRecovered.push_back(RecoveredToday - prevRecovered[Province.back()+Country.back()]);
               Recovered.push_back(RecoveredToday);
               prevRecovered[Province.back()+Country.back()] = RecoveredToday;
            }
            cnt++;
         }
         if (thelineT.Contains("\"") )
               cout << Province.back() << " " << Country.back() << " " << LastUpdated.back() << " " << Cases.back() << " " << Deaths.back() << " " << Recovered.back() << " " << DeltaCases.back() << " " << DeltaDeaths.back() << " " << DeltaRecovered.back() << endl;

      }
      cout << parsedate.Data() << ": " << totCases << ", " << totDeaths << ", " << totRecovered << endl;

      tr.Fill();

      Province.clear();
      Country.clear();
      LastUpdated.clear();
      Cases.clear();
      Deaths.clear();
      Recovered.clear();
      DeltaCases.clear();
      DeltaDeaths.clear();
      DeltaRecovered.clear();
      totCases = 0;
      totDeaths = 0;
      totRecovered = 0;

      ifs.close();
   }

   f.Write();
   f.Close();
}

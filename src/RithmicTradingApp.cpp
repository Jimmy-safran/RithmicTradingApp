/*   =====================================================================

Copyright (c) 2024 by Omnesys Technologies, Inc.  All rights reserved.

Warning :
        This Software Product is protected by copyright law and international
        treaties.  Unauthorized use, reproduction or distribution of this
        Software Product (including its documentation), or any portion of it,
        may result in severe civil and criminal penalties, and will be
        prosecuted to the maximum extent possible under the law.

        Omnesys Technologies, Inc. will compensate individuals providing
        admissible evidence of any unauthorized use, reproduction, distribution
        or redistribution of this Software Product by any person, company or 
        organization.

This Software Product is licensed strictly in accordance with a separate
Software System License Agreement, granted by Omnesys Technologies, Inc., which
contains restrictions on use, reverse engineering, disclosure, confidentiality 
and other matters.

     =====================================================================   */
/*   =====================================================================
     Compile/link commands for linux and darwin using R | API+.  These should 
     work if your pwd is the ./samples directory.  You may need to change the 
     name of the RApi library if you are using one of the library variants, 
     like R | API or R | Diamond API.

     64-bit linux (2.6.32 kernel) :

     g++ -O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing -I../include -o SampleOrder ../samples/SampleOrder.cpp -L../linux-gnu-2.6.32-x86_64/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -L/usr/kerberos/lib -lkrb5 -lk5crypto -lcom_err -lresolv -lm -lpthread -lrt

     64-bit linux (3.10.0 kernel) :

     g++ -O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing -I../include -o SampleOrder ../samples/SampleOrder.cpp -L../linux-gnu-3.10.0-x86_64/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -lpthread -lrt -ldl

     64-bit linux (4.18 kernel) :

     g++ -O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing -I../include -o SampleOrder ../samples/SampleOrder.cpp -L../linux-gnu-4.18-x86_64/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -lpthread -lrt -ldl

     64-bit darwin :

     g++ -O3 -D_REENTRANT -Wall -Wno-sign-compare -fno-strict-aliasing -Wpointer-arith -Winline -Wno-deprecated -Wno-write-strings -I../include -o ./SampleOrder ../samples/SampleOrder.cpp -L../darwin-10/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib -lz -Wl,-search_paths_first

     64-bit darwin arm64 :

     g++ -O3 -D_REENTRANT -Wall -Wno-sign-compare -fno-strict-aliasing -Wpointer-arith -Winline -Wno-deprecated -Wno-write-strings -I../include -o ./SampleOrder ../samples/SampleOrder.cpp -L../darwin-20.6-arm64/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib -lz

     =====================================================================   */

#include "RApiPlus.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <ctype.h>

#ifdef _WIN32
#define WinOS
#endif

#ifndef WinOS
#include <unistd.h>
#else
#include <Windows.h>
#endif

#define GOOD 0
#define BAD  1

using namespace RApi;

/*   =====================================================================   */
/*   Use global variables to share between the callback thread and main      */
/*   thread.  The booleans are a primitive method of signaling state         */
/*   between the two threads.                                                */

bool        g_bTsLoginComplete = false;
bool        g_bLoginFailed     = false;
bool        g_bRcvdAccount     = false;
bool        g_bRcvdPriceIncr   = false;
bool        g_bRcvdTradeRoutes = false;
bool        g_bDone            = false;

int         g_iToExchSsboe     = 0;
int         g_iToExchUsecs     = 0;
int         g_iFromExchSsboe   = 0;
int         g_iFromExchUsecs   = 0;

const int   g_iMAX_LEN         = 256;
char        g_cAccountId[g_iMAX_LEN];
char        g_cFcmId[g_iMAX_LEN];
char        g_cIbId[g_iMAX_LEN];
char        g_cExchange[g_iMAX_LEN];
char        g_cTradeRoute[g_iMAX_LEN];
AccountInfo g_oAccount;
tsNCharcb   g_sExchange;
tsNCharcb   g_sTradeRoute      = {(char *)NULL, 0};

REngine *   g_pEngine;

// Config file reading function
std::map<std::string, std::string> readConfigFile(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;
    std::string currentSection;
    
    if (!file.is_open()) {
        printf("Warning: Could not open config file: %s\n", filename.c_str());
        return config;
    }
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line.find(']') != std::string::npos) {
            currentSection = line.substr(1, line.find(']') - 1);
            continue;
        }
        
        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Remove leading/trailing whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Store with section prefix if not in global section
            if (!currentSection.empty()) {
                config[currentSection + "." + key] = value;
            } else {
                config[key] = value;
            }
        }
    }
    
    file.close();
         return config;
}

// Forward declarations
class MyAdmCallbacks;
class MyCallbacks;

// Helper function to cleanup resources
void cleanupResources(REngine* g_pEngine, RCallbacks* pCallbacks, MyAdmCallbacks* pAdmCallbacks, char** fake_envp);

int main(int      argc,
         char * * argv,
         char * * envp);

// Real trading function declarations
int sendOpenLongOrder(const char* exchange, const char* ticker, double price, int quantity);
int sendOpenShortOrder(const char* exchange, const char* ticker, double price, int quantity);
int sendRealTimeExitOrder(const char* exchange, const char* ticker, const char* side, double price, int quantity);
int sendStopOrder(const char* exchange, const char* ticker, const char* side, double stopPrice, int quantity);
int sendModifyOrder(const char* exchange, const char* ticker, const char* orderNum, double newPrice, int newQuantity);
int sendCancelOrder(const char* exchange, const char* ticker, const char* orderNum);
int sendBracketOrder(const char* exchange, const char* ticker, const char* side, double entryPrice, double stopPrice, double targetPrice, int quantity);
int sendTrailingStopOrder(const char* exchange, const char* ticker, const char* side, double stopPrice, int trailTicks, int quantity);
int sendTimeBasedOrder(const char* exchange, const char* ticker, const char* side, double price, int quantity, const char* duration);

/*   =====================================================================   */
/*                          class declarations                               */
/*   =====================================================================   */

class MyAdmCallbacks: public AdmCallbacks
     {
     public :
     MyAdmCallbacks()  {};
     ~MyAdmCallbacks() {};

     /*   ----------------------------------------------------------------   */

     virtual int Alert(AlertInfo * pInfo,
                       void *      pContext,
                       int *       aiCode);
     };

/*   =====================================================================   */

class MyCallbacks: public RCallbacks
     {
     public :
     MyCallbacks()  {};
     ~MyCallbacks() {};

     /*   ----------------------------------------------------------------   */

     virtual int Alert(AlertInfo * pInfo,
                       void *      pContext,
                       int *       aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int AskQuote(AskInfo * pInfo,
                          void *    pContext,
                          int *     aiCode);

     virtual int BestAskQuote(AskInfo * pInfo,
                              void *    pContext,
                              int *     aiCode);

     virtual int BestBidAskQuote(BidInfo * pBid,
				 AskInfo * pAsk,
				 void *    pContext,
				 int *     aiCode);

     virtual int BestBidQuote(BidInfo * pInfo,
                              void *    pContext,
                              int *     aiCode);

     virtual int BidQuote(BidInfo * pInfo,
                          void *    pContext,
                          int *     aiCode);

     virtual int BinaryContractList(BinaryContractListInfo * pInfo,
				    void *                   pContext,
				    int *                    aiCode);

     virtual int ClosePrice(ClosePriceInfo * pInfo,
                            void *           pContext,
                            int *            aiCode);

     virtual int ClosingIndicator(ClosingIndicatorInfo * pInfo,
                                  void *                 pContext,
                                  int *                  aiCode);

     virtual int EndQuote(EndQuoteInfo * pInfo,
                          void *         pContext,
                          int *          aiCode);

     virtual int EquityOptionStrategyList(EquityOptionStrategyListInfo * pInfo,
					  void *                         pContext,
					  int *                          aiCode);

     virtual int HighPrice(HighPriceInfo * pInfo,
                           void *          pContext,
                           int *           aiCode);

     virtual int InstrumentByUnderlying(InstrumentByUnderlyingInfo * pInfo,
					void *                       pContext,
					int *                        aiCode);

     virtual int InstrumentSearch(InstrumentSearchInfo * pInfo,
				  void *                 pContext,
				  int *                  aiCode);

     virtual int LimitOrderBook(LimitOrderBookInfo * pInfo,
                                void *               pContext,
                                int *                aiCode);

     virtual int LowPrice(LowPriceInfo * pInfo,
                          void *         pContext,
                          int *          aiCode);

     virtual int MarketMode(MarketModeInfo * pInfo,
                            void *           pContext,
                            int *            aiCode);

     virtual int OpenInterest(OpenInterestInfo * pInfo,
			      void *             pContext,
			      int *              aiCode);

     virtual int OpenPrice(OpenPriceInfo * pInfo,
                           void *          pContext,
                           int *           aiCode);

     virtual int OpeningIndicator(OpeningIndicatorInfo * pInfo,
				  void *                 pContext,
				  int *                  aiCode);

     virtual int OptionList(OptionListInfo * pInfo,
                            void *           pContext,
                            int *            aiCode);

     virtual int RefData(RefDataInfo * pInfo,
                         void *        pContext,
                         int *         aiCode);

     virtual int SettlementPrice(SettlementPriceInfo * pInfo,
                                 void *                pContext,
                                 int *                 aiCode);

     virtual int Strategy(StrategyInfo * pInfo,
			  void *         pContext,
			  int *          aiCode);

     virtual int StrategyList(StrategyListInfo * pInfo,
			      void *             pContext,
			      int *              aiCode);

     virtual int TradeCondition(TradeInfo * pInfo,
                                void *      pContext,
                                int *       aiCode);

     virtual int TradePrint(TradeInfo * pInfo,
                            void *      pContext,
                            int *       aiCode);

     virtual int TradeReplay(TradeReplayInfo * pInfo,
			     void *            pContext,
			     int *             aiCode);

     virtual int TradeRoute(TradeRouteInfo * pInfo,
			    void *           pContext,
			    int *            aiCode);

     virtual int TradeRouteList(TradeRouteListInfo * pInfo,
				void *               pContext,
				int *                aiCode);

     virtual int TradeVolume(TradeVolumeInfo * pInfo,
                             void *            pContext,
                             int *             aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int Bar(BarInfo * pInfo,
		     void *    pContext,
		     int *     aiCode);

     virtual int BarReplay(BarReplayInfo * pInfo,
			   void *          pContext,
			   int *           aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int AccountList(AccountListInfo * pInfo,
                             void *            pContext,
                             int *             aiCode);

     virtual int PasswordChange(PasswordChangeInfo * pInfo,
				void *               pContext,
				int *                aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int ExchangeList(ExchangeListInfo * pInfo,
			      void *             pContext,
			      int *              aiCode);

     virtual int ExecutionReplay(ExecutionReplayInfo * pInfo,
                                 void *                pContext,
                                 int *                 aiCode);

     virtual int LineUpdate(LineInfo * pInfo,
                            void *     pContext,
                            int *      aiCode);

     virtual int OpenOrderReplay(OrderReplayInfo * pInfo,
                                 void *            pContext,
                                 int *             aiCode);

     virtual int OrderReplay(OrderReplayInfo * pInfo,
                             void *            pContext,
                             int *             aiCode);

     virtual int PnlReplay(PnlReplayInfo * pInfo,
                           void *          pContext,
                           int *           aiCode);

     virtual int PnlUpdate(PnlInfo * pInfo,
                           void *    pContext,
                           int *     aiCode);

     virtual int PriceIncrUpdate(PriceIncrInfo * pInfo,
                                 void *          pContext,
                                 int *           aiCode);

     virtual int ProductRmsList(ProductRmsListInfo * pInfo,
				void *               pContext,
				int *                aiCode);

     virtual int SingleOrderReplay(SingleOrderReplayInfo * pInfo,
				   void *                  pContext,
				   int *                   aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int BustReport(OrderBustReport * pReport,
                            void *            pContext,
                            int *             aiCode);

     virtual int CancelReport(OrderCancelReport * pReport,
                              void *              pContext,
                              int *               aiCode);

     virtual int FailureReport(OrderFailureReport * pReport,
                               void *               pContext,
                               int *                aiCode);

     virtual int FillReport(OrderFillReport * pReport,
                            void *            pContext,
                            int *             aiCode);

     virtual int ModifyReport(OrderModifyReport * pReport,
                              void *              pContext,
                              int *               aiCode);

     virtual int NotCancelledReport(OrderNotCancelledReport * pReport,
                                    void *                    pContext,
                                    int *                     aiCode);

     virtual int NotModifiedReport(OrderNotModifiedReport * pReport,
                                   void *                   pContext,
                                   int *                    aiCode);

     virtual int RejectReport(OrderRejectReport * pReport,
                              void *              pContext,
                              int *               aiCode);

     virtual int StatusReport(OrderStatusReport * pReport,
                              void *              pContext,
                              int *               aiCode);

     virtual int TradeCorrectReport(OrderTradeCorrectReport * pReport,
                                    void *                    pContext,
                                    int *                     aiCode);

     virtual int TriggerPulledReport(OrderTriggerPulledReport * pReport,
                                     void *                     pContext,
                                     int *                      aiCode);

     virtual int TriggerReport(OrderTriggerReport * pReport,
                               void *              pContext,
                               int *               aiCode);

     virtual int OtherReport(OrderReport * pReport,
                             void *        pContext,
                             int *         aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int SodUpdate(SodReport * pReport,
                           void *      pContext,
                           int *       aiCode);

     /*   ----------------------------------------------------------------   */

     virtual int Quote(QuoteReport * pReport,
		       void *        pContext,
		       int *         aiCode);

     /*   ----------------------------------------------------------------   */

     private :
     };

/*   =====================================================================   */
/*                          class definitions                                */
/*   =====================================================================   */

// Helper function to cleanup resources implementation
void cleanupResources(REngine* g_pEngine, RCallbacks* pCallbacks, MyAdmCallbacks* pAdmCallbacks, char** fake_envp) {
     if (g_pEngine != NULL) {
          delete g_pEngine;
     }
     if (pCallbacks != NULL) {
          delete pCallbacks;
     }
     if (pAdmCallbacks != NULL) {
          delete pAdmCallbacks;
     }
     
     // Clean up dynamically allocated environment variables
     if (fake_envp != NULL) {
          for (int i = 0; i < 8; i++) {
               if (fake_envp[i] != NULL) {
                    delete[] fake_envp[i];
               }
          }
          delete[] fake_envp;
     }
}

int MyAdmCallbacks::Alert(AlertInfo * pInfo,
                          void *      pContext,
                          int *       aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\n");
     if (!pInfo -> dump(&iIgnored))
          {
          printf("error in pInfo -> dump : %d", iIgnored);
          }

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::AccountList(AccountListInfo * pInfo,
                             void *            pContext,
                             int *             aiCode)
     {
     AccountInfo * pAccount;
     int iCode;

     /*   ----------------------------------------------------------------   */

     if (pInfo -> iArrayLen > 0)
          {
          pAccount = &pInfo -> asAccountInfoArray[0];

          /* copy the first account */
          if ((pAccount -> sAccountId.iDataLen > g_iMAX_LEN) ||
              (pAccount -> sFcmId.iDataLen     > g_iMAX_LEN) ||
              (pAccount -> sIbId.iDataLen      > g_iMAX_LEN))
               {
               printf("Error: Account data too large\n");
               }
          else
               {
               memcpy(&g_cAccountId, 
                      pAccount -> sAccountId.pData,
                      pAccount -> sAccountId.iDataLen);

               g_oAccount.sAccountId.pData    = g_cAccountId;
               g_oAccount.sAccountId.iDataLen = pAccount -> sAccountId.iDataLen;

               /*   ------------------------------------------------------   */

               memcpy(&g_cFcmId, 
                      pAccount -> sFcmId.pData,
                      pAccount -> sFcmId.iDataLen);
               g_oAccount.sFcmId.pData    = g_cFcmId;
               g_oAccount.sFcmId.iDataLen = pAccount -> sFcmId.iDataLen;

               /*   ------------------------------------------------------   */

               memcpy(&g_cIbId, 
                      pAccount -> sIbId.pData,
                      pAccount -> sIbId.iDataLen);
               g_oAccount.sIbId.pData    = g_cIbId;
               g_oAccount.sIbId.iDataLen = pAccount -> sIbId.iDataLen;

               if (!g_pEngine -> subscribeOrder(&g_oAccount, &iCode))
                    {
                    printf("Error: subscribeOrder() failed: %d\n", iCode);
                    }

               printf("+ Account: %.*s (%.*s/%.*s)\n", 
                      g_oAccount.sAccountId.iDataLen, g_cAccountId,
                      g_oAccount.sFcmId.iDataLen, g_cFcmId,
                      g_oAccount.sIbId.iDataLen, g_cIbId);
               g_bRcvdAccount = true;
               }
          }

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::PasswordChange(PasswordChangeInfo * pInfo,
				void *               pContext,
				int *                aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::Alert(AlertInfo * pInfo,
                       void *      pContext,
                       int *       aiCode)
     {
     /*   ----------------------------------------------------------------   */
     /*   signal when the login to the trading system is complete */

     if (pInfo -> iAlertType == ALERT_LOGIN_COMPLETE &&
         pInfo -> iConnectionId == TRADING_SYSTEM_CONNECTION_ID)
          {
          printf("+ Trading System Login Complete\n");
          g_bTsLoginComplete = true;
          }

     /*   ----------------------------------------------------------------   */
     /*   handle login failures */

     if (pInfo -> iAlertType == ALERT_LOGIN_FAILED)
          {
          printf("ERROR: Login Failed - Connection ID: %d, RP Code: %d\n", 
                 pInfo->iConnectionId, pInfo->iRpCode);
          if (pInfo->sMessage.iDataLen > 0) {
               printf("ERROR: %.*s\n", pInfo->sMessage.iDataLen, pInfo->sMessage.pData);
          }
          g_bLoginFailed = true;
          g_bTsLoginComplete = true; // Set to true to break out of the wait loop
          }

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::ExchangeList(ExchangeListInfo * pInfo,
			      void *             pContext,
			      int *              aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::ExecutionReplay(ExecutionReplayInfo * pInfo,
                                 void *                pContext,
                                 int *                 aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }


/*   =====================================================================   */

int MyCallbacks::LineUpdate(LineInfo * pInfo,
                            void *     pContext,
                            int *      aiCode)
     {
     tsNCharcb sOrderSentToExch = {(char *)"order sent to exch",
                                   (int)(int)strlen("order sent to exch")};

     /*   ----------------------------------------------------------------   */
     /*   record when the order was sent to the exchange... */

     if (pInfo -> sStatus.iDataLen == sOrderSentToExch.iDataLen &&
	 memcmp(pInfo -> sStatus.pData, 
		sOrderSentToExch.pData, 
		sOrderSentToExch.iDataLen) == 0)
	  {
	  g_iToExchSsboe = pInfo -> iSsboe;
	  g_iToExchUsecs = pInfo -> iUsecs;
	  printf("→ Order sent to exchange\n");
	  printf("DEBUG: LineUpdate - Order Number: %.*s\n", pInfo->sOrderNum.iDataLen, pInfo->sOrderNum.pData);
	  printf("DEBUG: LineUpdate - Order Number Length: %d\n", pInfo->sOrderNum.iDataLen);
	  printf("DEBUG: LineUpdate - Order Number Raw: '");
	  for (int i = 0; i < pInfo->sOrderNum.iDataLen; i++) {
	      printf("%c", pInfo->sOrderNum.pData[i]);
	  }
	  printf("'\n");
	  }

     /*   ----------------------------------------------------------------   */
     /*   if there's a completion reason, the order is complete... */

     if (pInfo -> sCompletionReason.pData)
          {
          printf("-> Order complete: %.*s\n", 
                 pInfo->sCompletionReason.iDataLen, pInfo->sCompletionReason.pData);
          printf("DEBUG: LineUpdate Complete - Order Number: %.*s\n", pInfo->sOrderNum.iDataLen, pInfo->sOrderNum.pData);
          g_bDone = true;
          }

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }


/*   =====================================================================   */

int MyCallbacks::OpenOrderReplay(OrderReplayInfo * pInfo,
                                 void *            pContext,
                                 int *             aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }


/*   =====================================================================   */

int MyCallbacks::OrderReplay(OrderReplayInfo * pInfo,
                             void *            pContext,
                             int *             aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }


/*   =====================================================================   */

int MyCallbacks::PnlReplay(PnlReplayInfo * pInfo,
                           void *          pContext,
                           int *           aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::PnlUpdate(PnlInfo * pInfo,
                           void *    pContext,
                           int *     aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::PriceIncrUpdate(PriceIncrInfo * pInfo,
                                 void *          pContext,
                                 int *           aiCode)
     {
     double tickSize = 0.0;
     if (pInfo->iArrayLen > 0) {
         tickSize = pInfo->asPriceIncrArray[0].dPriceIncr;
     }
     printf("+ Price Info: %.*s %.*s (tick: %.2f)\n", 
            pInfo->sExchange.iDataLen, pInfo->sExchange.pData,
            pInfo->sTicker.iDataLen, pInfo->sTicker.pData, 
            tickSize);
     g_bRcvdPriceIncr = true;

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::ProductRmsList(ProductRmsListInfo * pInfo,
				void *               pContext,
				int *                aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::SingleOrderReplay(SingleOrderReplayInfo * pInfo,
				   void *                  pContext,
				   int *                   aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::BustReport(OrderBustReport * pReport,
                            void *            pContext,
                            int *             aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Bust Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::CancelReport(OrderCancelReport * pReport,
                              void *              pContext,
                              int *               aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Cancel Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::FailureReport(OrderFailureReport * pReport,
                               void *               pContext,
                               int *                aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Failure Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::FillReport(OrderFillReport * pReport,
                            void *            pContext,
                            int *             aiCode)
     {
     printf("[FILLED] ORDER FILLED: %lld @ %.2f\n", 
            pReport->llTotalFilled, pReport->dFillPrice);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::ModifyReport(OrderModifyReport * pReport,
                              void *              pContext,
                              int *               aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Modify Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::NotCancelledReport(OrderNotCancelledReport * pReport,
                                    void *                    pContext,
                                    int *                     aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived NotCancelled Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::NotModifiedReport(OrderNotModifiedReport * pReport,
                                   void *                   pContext,
                                   int *                    aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived NotModified Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::RejectReport(OrderRejectReport * pReport,
                              void *              pContext,
                              int *               aiCode)
     {
     /*   ----------------------------------------------------------------   */
     /*   record when the order returned from the exchange... */

     g_iFromExchSsboe = pReport -> iSsboe;
     g_iFromExchUsecs = pReport -> iUsecs;

     printf("[REJECTED] ORDER REJECTED: %.*s\n", 
            pReport->sText.iDataLen, pReport->sText.pData);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::StatusReport(OrderStatusReport * pReport,
                              void *              pContext,
                              int *               aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Status Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */
     /*   record when the order returned from the exchange... */

     g_iFromExchSsboe = pReport -> iSsboe;
     g_iFromExchUsecs = pReport -> iUsecs;

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradeCorrectReport(OrderTradeCorrectReport * pReport,
                                    void *                    pContext,
                                    int *                     aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Trade Correct Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::TriggerPulledReport(OrderTriggerPulledReport * pReport,
                                     void *                     pContext,
                                     int *                      aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Trigger Pulled Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::TriggerReport(OrderTriggerReport * pReport,
                               void *               pContext,
                               int *                aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Trigger Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::OtherReport(OrderReport * pReport,
                             void *        pContext,
                             int *         aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Other Report\n");
     printf("DEBUG: Order Report Details:\n");
     printf("  - Order Number: %.*s\n", pReport->sOrderNum.iDataLen, pReport->sOrderNum.pData);
     printf("  - Exchange: %.*s\n", pReport->sExchange.iDataLen, pReport->sExchange.pData);
     printf("  - Ticker: %.*s\n", pReport->sTicker.iDataLen, pReport->sTicker.pData);
     printf("  - Buy/Sell: %.*s\n", pReport->sBuySellType.iDataLen, pReport->sBuySellType.pData);
     printf("  - Order Number Length: %d\n", pReport->sOrderNum.iDataLen);
     printf("  - Order Number Raw: '");
     for (int i = 0; i < pReport->sOrderNum.iDataLen; i++) {
         printf("%c", pReport->sOrderNum.pData[i]);
     }
     printf("'\n");
     
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::SodUpdate(SodReport * pReport,
                           void *      pContext,
                           int *       aiCode)
     {
     int iIgnored;

     /*   ----------------------------------------------------------------   */

     printf("\n\nReceived Sod Report\n");
     pReport -> dump(&iIgnored);

     /*   ----------------------------------------------------------------   */

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::Quote(QuoteReport * pReport,
		       void *        pContext,
		       int *         aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::AskQuote(AskInfo * pInfo,
                          void *    pContext,
                          int *     aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::BestAskQuote(AskInfo * pInfo,
                              void *    pContext,
                              int *     aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::BestBidAskQuote(BidInfo * pBid,
				 AskInfo * pAsk,
				 void *    pContext,
				 int *     aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::BestBidQuote(BidInfo * pInfo,
                              void *    pContext,
                              int *     aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::BidQuote(BidInfo * pInfo,
                          void *    pContext,
                          int *     aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::BinaryContractList(BinaryContractListInfo * pInfo,
				    void *                   pContext,
				    int *                    aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::ClosePrice(ClosePriceInfo * pInfo,
                            void *           pContext,
                            int *            aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::ClosingIndicator(ClosingIndicatorInfo * pInfo,
                                  void *                 pContext,
                                  int *                  aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::EndQuote(EndQuoteInfo * pInfo,
			  void *         pContext,
			  int *          aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::EquityOptionStrategyList(EquityOptionStrategyListInfo * pInfo,
					  void *                         pContext,
					  int *                          aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::HighPrice(HighPriceInfo * pInfo,
                           void *          pContext,
                           int *           aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::InstrumentByUnderlying(InstrumentByUnderlyingInfo * pInfo,
					void *                       pContext,
					int *                        aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::InstrumentSearch(InstrumentSearchInfo * pInfo,
				  void *                 pContext,
				  int *                  aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::LimitOrderBook(LimitOrderBookInfo * pInfo,
                                void *               pContext,
                                int *                aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::LowPrice(LowPriceInfo * pInfo,
                          void *         pContext,
                          int *          aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::MarketMode(MarketModeInfo * pInfo,
                            void *           pContext,
                            int *            aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::OpenInterest(OpenInterestInfo * pInfo,
			      void *             pContext,
			      int *              aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::OpenPrice(OpenPriceInfo * pInfo,
                           void *          pContext,
                           int *           aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::OpeningIndicator(OpeningIndicatorInfo * pInfo,
				  void *                 pContext,
				  int *                  aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::OptionList(OptionListInfo * pInfo,
			    void *           pContext,
			    int *            aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::RefData(RefDataInfo * pInfo,
                         void *        pContext,
                         int *         aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::SettlementPrice(SettlementPriceInfo * pInfo,
                                 void *                pContext,
                                 int *                 aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::Strategy(StrategyInfo * pInfo,
			  void *         pContext,
			  int *          aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::StrategyList(StrategyListInfo * pInfo,
			      void *             pContext,
			      int *              aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradeCondition(TradeInfo * pInfo,
                                void *      pContext,
                                int *       aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradePrint(TradeInfo * pInfo,
                            void *      pContext,
                            int *       aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradeReplay(TradeReplayInfo * pInfo,
			     void *            pContext,
			     int *             aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradeRoute(TradeRouteInfo * pInfo,
			    void *           pContext,
			    int *            aiCode)
     {
     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradeRouteList(TradeRouteListInfo * pInfo,
				void *               pContext,
				int *                aiCode)
     {
     tsNCharcb sFcmId;
     tsNCharcb sIbId;
     tsNCharcb sExchange;
     tsNCharcb sTradeRoute;
     tsNCharcb sStatus;

     for (int i = 0; i < pInfo -> iArrayLen; i++)
	  {
	  sFcmId      = pInfo -> asTradeRouteInfoArray[i].sFcmId;
	  sIbId       = pInfo -> asTradeRouteInfoArray[i].sIbId;
	  sExchange   = pInfo -> asTradeRouteInfoArray[i].sExchange;
	  sTradeRoute = pInfo -> asTradeRouteInfoArray[i].sTradeRoute;
	  sStatus     = pInfo -> asTradeRouteInfoArray[i].sStatus;

	  /* use first trade route where fcm/ib/exch matches, and status is "UP" */
	  if (g_oAccount.sFcmId.iDataLen == sFcmId.iDataLen &&
	      (memcmp(g_oAccount.sFcmId.pData, 
		      sFcmId.pData, 
		      g_oAccount.sFcmId.iDataLen) == 0) &&

	      g_oAccount.sIbId.iDataLen == sIbId.iDataLen &&
	      (memcmp(g_oAccount.sIbId.pData, 
		      sIbId.pData, 
		      g_oAccount.sIbId.iDataLen) == 0) &&

	      g_sExchange.iDataLen == sExchange.iDataLen &&
	      (memcmp(g_sExchange.pData, 
		      sExchange.pData, 
		      g_sExchange.iDataLen) == 0) &&

	      sTRADE_ROUTE_STATUS_UP.iDataLen == sStatus.iDataLen &&
	      (memcmp(sTRADE_ROUTE_STATUS_UP.pData, 
		      sStatus.pData, 
		      sTRADE_ROUTE_STATUS_UP.iDataLen) == 0))
	       {
	       /*   copy memory into global trade route string */
	       memcpy(&g_cTradeRoute, 
		      sTradeRoute.pData,
		      sTradeRoute.iDataLen);
	       
	       g_sTradeRoute.pData    = g_cTradeRoute;
	       g_sTradeRoute.iDataLen = sTradeRoute.iDataLen;

	                      printf("+ Trade Route: %.*s::%.*s::%.*s::%.*s::%.*s\n",
		      sFcmId.iDataLen, sFcmId.pData, 
		      sIbId.iDataLen, sIbId.pData, 
		      sExchange.iDataLen, sExchange.pData, 
		      sTradeRoute.iDataLen, sTradeRoute.pData, 
		      sStatus.iDataLen, sStatus.pData);
	       break;
	       }

	  g_sTradeRoute.pData    = NULL;
	  g_sTradeRoute.iDataLen = 0;
	  }

     g_bRcvdTradeRoutes = true;

     *aiCode = API_OK;
     return(OK);
     }

/*   =====================================================================   */

int MyCallbacks::TradeVolume(TradeVolumeInfo * pInfo,
                             void *            pContext,
                             int *             aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::Bar(BarInfo * pInfo,
		     void *    pContext,
		     int *     aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }

/*   =====================================================================   */

int MyCallbacks::BarReplay(BarReplayInfo * pInfo,
			   void *          pContext,
			   int *           aiCode)
     {
     *aiCode = API_OK;
     return (OK);
     }
/*   =====================================================================   */

// Real trading functions that connect to Rithmic system
int sendOpenLongOrder(const char* exchange, const char* ticker, double price, int quantity) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    // If price is 0.0, treat as market order
    if (price == 0.0) {
        printf("Sending OPEN LONG MARKET order for %s %s, quantity %d\n", exchange, ticker, quantity);
        
        MarketOrderParams marketParams;
        marketParams.pAccount = &g_oAccount;
        marketParams.sExchange.pData = const_cast<char*>(exchange);
        marketParams.sExchange.iDataLen = (int)strlen(exchange);
        marketParams.sTicker.pData = const_cast<char*>(ticker);
        marketParams.sTicker.iDataLen = (int)strlen(ticker);
        marketParams.sBuySellType.pData = const_cast<char*>("B");
        marketParams.sBuySellType.iDataLen = 1;
        marketParams.sDuration = sORDER_DURATION_DAY;
        marketParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
        marketParams.iQty = quantity;
        marketParams.sTradeRoute = g_sTradeRoute;
        
        int iCode;
        if (!g_pEngine->sendOrder(&marketParams, &iCode)) {
            printf("Error sending OPEN LONG MARKET order: %d\n", iCode);
            return BAD;
        }
        
        printf("OPEN LONG MARKET order sent successfully\n");
        return GOOD;
    } else {
        printf("Sending OPEN LONG LIMIT order for %s %s at price %.2f, quantity %d\n", exchange, ticker, price, quantity);
        
        LimitOrderParams limitParams;
        limitParams.pAccount = &g_oAccount;
        limitParams.sExchange.pData = const_cast<char*>(exchange);
        limitParams.sExchange.iDataLen = (int)strlen(exchange);
        limitParams.sTicker.pData = const_cast<char*>(ticker);
        limitParams.sTicker.iDataLen = (int)strlen(ticker);
        limitParams.sBuySellType.pData = const_cast<char*>("B");
        limitParams.sBuySellType.iDataLen = 1;
        limitParams.sDuration = sORDER_DURATION_DAY;
        limitParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
        limitParams.iQty = quantity;
        limitParams.dPrice = price;
        limitParams.sTradeRoute = g_sTradeRoute;
        
        int iCode;
        if (!g_pEngine->sendOrder(&limitParams, &iCode)) {
            printf("Error sending OPEN LONG LIMIT order: %d\n", iCode);
            return BAD;
        }
        
        printf("OPEN LONG LIMIT order sent successfully\n");
        return GOOD;
    }
}

int sendOpenShortOrder(const char* exchange, const char* ticker, double price, int quantity) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    // If price is 0.0, treat as market order
    if (price == 0.0) {
        printf("Sending OPEN SHORT MARKET order for %s %s, quantity %d\n", exchange, ticker, quantity);
        
        MarketOrderParams marketParams;
        marketParams.pAccount = &g_oAccount;
        marketParams.sExchange.pData = const_cast<char*>(exchange);
        marketParams.sExchange.iDataLen = (int)strlen(exchange);
        marketParams.sTicker.pData = const_cast<char*>(ticker);
        marketParams.sTicker.iDataLen = (int)strlen(ticker);
        marketParams.sBuySellType.pData = const_cast<char*>("S");
        marketParams.sBuySellType.iDataLen = 1;
        marketParams.sDuration = sORDER_DURATION_DAY;
        marketParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
        marketParams.iQty = quantity;
        marketParams.sTradeRoute = g_sTradeRoute;
        
        int iCode;
        if (!g_pEngine->sendOrder(&marketParams, &iCode)) {
            printf("Error sending OPEN SHORT MARKET order: %d\n", iCode);
            return BAD;
        }
        
        printf("OPEN SHORT MARKET order sent successfully\n");
        return GOOD;
    } else {
        printf("Sending OPEN SHORT LIMIT order for %s %s at price %.2f, quantity %d\n", exchange, ticker, price, quantity);
        
        LimitOrderParams limitParams;
        limitParams.pAccount = &g_oAccount;
        limitParams.sExchange.pData = const_cast<char*>(exchange);
        limitParams.sExchange.iDataLen = (int)strlen(exchange);
        limitParams.sTicker.pData = const_cast<char*>(ticker);
        limitParams.sTicker.iDataLen = (int)strlen(ticker);
        limitParams.sBuySellType.pData = const_cast<char*>("S");
        limitParams.sBuySellType.iDataLen = 1;
        limitParams.sDuration = sORDER_DURATION_DAY;
        limitParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
        limitParams.iQty = quantity;
        limitParams.dPrice = price;
        limitParams.sTradeRoute = g_sTradeRoute;
        
        int iCode;
        if (!g_pEngine->sendOrder(&limitParams, &iCode)) {
            printf("Error sending OPEN SHORT LIMIT order: %d\n", iCode);
            return BAD;
        }
        
        printf("OPEN SHORT LIMIT order sent successfully\n");
        return GOOD;
    }
}

int sendRealTimeExitOrder(const char* exchange, const char* ticker, const char* side, double price, int quantity) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending REAL TIME EXIT order for %s %s %s at price %.2f, quantity %d\n", side, exchange, ticker, price, quantity);
    
    MarketOrderParams marketParams;
    marketParams.pAccount = &g_oAccount;
    marketParams.sExchange.pData = const_cast<char*>(exchange);
    marketParams.sExchange.iDataLen = (int)strlen(exchange);
    marketParams.sTicker.pData = const_cast<char*>(ticker);
    marketParams.sTicker.iDataLen = (int)strlen(ticker);
    marketParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "S" : "B");
    marketParams.sBuySellType.iDataLen = 1;
    marketParams.sDuration = sORDER_DURATION_DAY;
    marketParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    marketParams.iQty = quantity;
    marketParams.sTradeRoute = g_sTradeRoute;
    
    int iCode;
    if (!g_pEngine->sendOrder(&marketParams, &iCode)) {
        printf("Error sending REAL TIME EXIT order: %d\n", iCode);
        return BAD;
    }
    
    printf("REAL TIME EXIT order sent successfully\n");
    return GOOD;
}

int sendStopOrder(const char* exchange, const char* ticker, const char* side, double stopPrice, int quantity) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending STOP order for %s %s %s at stop price %.2f, quantity %d\n", side, exchange, ticker, stopPrice, quantity);
    
    StopMarketOrderParams stopParams;
    stopParams.pAccount = &g_oAccount;
    stopParams.sExchange.pData = const_cast<char*>(exchange);
    stopParams.sExchange.iDataLen = (int)strlen(exchange);
    stopParams.sTicker.pData = const_cast<char*>(ticker);
    stopParams.sTicker.iDataLen = (int)strlen(ticker);
    stopParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "S" : "B");
    stopParams.sBuySellType.iDataLen = 1;
    stopParams.sDuration = sORDER_DURATION_DAY;
    stopParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    stopParams.iQty = quantity;
    stopParams.dTriggerPrice = stopPrice;
    stopParams.sTradeRoute = g_sTradeRoute;
    
    int iCode;
    if (!g_pEngine->sendOrder(&stopParams, &iCode)) {
        printf("Error sending STOP order: %d\n", iCode);
        return BAD;
    }
    
    printf("STOP order sent successfully\n");
    return GOOD;
}

int sendModifyOrder(const char* exchange, const char* ticker, const char* orderNum, double newPrice, int newQuantity) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending MODIFY order for %s %s order %s to price %.2f, quantity %d\n", exchange, ticker, orderNum, newPrice, newQuantity);
    printf("DEBUG: Order number length: %d, Order number: '%s'\n", (int)strlen(orderNum), orderNum);
    printf("DEBUG: Account info - FCM: %.*s, IB: %.*s\n", 
           g_oAccount.sFcmId.iDataLen, g_oAccount.sFcmId.pData,
           g_oAccount.sIbId.iDataLen, g_oAccount.sIbId.pData);
    
    ModifyOrderParams modifyParams;
    modifyParams.pAccount = &g_oAccount;
    modifyParams.sExchange.pData = const_cast<char*>(exchange);
    modifyParams.sExchange.iDataLen = (int)strlen(exchange);
    modifyParams.sTicker.pData = const_cast<char*>(ticker);
    modifyParams.sTicker.iDataLen = (int)strlen(ticker);
    modifyParams.sOrderNum.pData = const_cast<char*>(orderNum);
    modifyParams.sOrderNum.iDataLen = (int)strlen(orderNum);
    modifyParams.bPrice = true;
    modifyParams.dPrice = newPrice;
    modifyParams.bQty = true;
    modifyParams.iQty = newQuantity;
    modifyParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    
    printf("DEBUG: ModifyParams - OrderNum: '%s', Price: %.2f, Qty: %d\n", 
           modifyParams.sOrderNum.pData, modifyParams.dPrice, modifyParams.iQty);
    
    int iCode;
    if (!g_pEngine->modifyOrder(&modifyParams, &iCode)) {
        printf("Error sending MODIFY order: %d\n", iCode);
        printf("DEBUG: Error code %d typically means:\n", iCode);
        printf("  - Order number not found\n");
        printf("  - Order already executed/cancelled\n");
        printf("  - Order belongs to different account\n");
        printf("  - Order is in invalid state for modification\n");
        return BAD;
    }
    
    printf("MODIFY order sent successfully\n");
    return GOOD;
}

int sendCancelOrder(const char* exchange, const char* ticker, const char* orderNum) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending CANCEL order for %s %s order %s\n", exchange, ticker, orderNum);
    printf("DEBUG: Order number length: %d, Order number: '%s'\n", (int)strlen(orderNum), orderNum);
    printf("DEBUG: Account info - FCM: %.*s, IB: %.*s\n", 
           g_oAccount.sFcmId.iDataLen, g_oAccount.sFcmId.pData,
           g_oAccount.sIbId.iDataLen, g_oAccount.sIbId.pData);
    
    tsNCharcb sOrderNum;
    sOrderNum.pData = const_cast<char*>(orderNum);
    sOrderNum.iDataLen = (int)strlen(orderNum);
    
    tsNCharcb sExchange;
    sExchange.pData = const_cast<char*>(exchange);
    sExchange.iDataLen = (int)strlen(exchange);
    
    tsNCharcb sTicker;
    sTicker.pData = const_cast<char*>(ticker);
    sTicker.iDataLen = (int)strlen(ticker);
    
    printf("DEBUG: tsNCharcb - pData: '%s', iDataLen: %d\n", sOrderNum.pData, sOrderNum.iDataLen);
    printf("DEBUG: Exchange: %.*s, Ticker: %.*s\n", sExchange.iDataLen, sExchange.pData, sTicker.iDataLen, sTicker.pData);
    
    int iCode;
    // Try with exchange and ticker parameters
    if (!g_pEngine->cancelOrder(&g_oAccount, &sOrderNum, &sExchange, &sTicker, NULL, NULL, &iCode)) {
        printf("Error sending CANCEL order: %d\n", iCode);
        printf("DEBUG: Error code %d typically means:\n", iCode);
        printf("  - Order number not found\n");
        printf("  - Order already executed/cancelled\n");
        printf("  - Order belongs to different account\n");
        printf("  - Order is in invalid state for cancellation\n");
        return BAD;
    }
    
    printf("CANCEL order sent successfully\n");
    return GOOD;
}

int sendBracketOrder(const char* exchange, const char* ticker, const char* side, double entryPrice, double stopPrice, double targetPrice, int quantity) {
    printf("DEBUG: sendBracketOrder called with: exchange=%s, ticker=%s, side=%s, entryPrice=%.2f, stopPrice=%.2f, targetPrice=%.2f, quantity=%d\n", 
           exchange, ticker, side, entryPrice, stopPrice, targetPrice, quantity);
    
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending BRACKET order for %s %s %s: entry %.2f, stop %.2f, target %.2f, qty %d\n", 
           side, exchange, ticker, entryPrice, stopPrice, targetPrice, quantity);
    
    // First order - entry (limit order)
    LimitOrderParams entryParams;
    entryParams.pAccount = &g_oAccount;
    entryParams.sExchange.pData = const_cast<char*>(exchange);
    entryParams.sExchange.iDataLen = (int)strlen(exchange);
    entryParams.sTicker.pData = const_cast<char*>(ticker);
    entryParams.sTicker.iDataLen = (int)strlen(ticker);
    entryParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "B" : "S");
    entryParams.sBuySellType.iDataLen = 1;
    entryParams.sDuration = sORDER_DURATION_DAY;
    entryParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    entryParams.iQty = quantity;
    entryParams.dPrice = entryPrice;
    entryParams.sTradeRoute = g_sTradeRoute;
    
    int iCode;
    if (!g_pEngine->sendOrder(&entryParams, &iCode)) {
        printf("Error sending BRACKET entry order: %d\n", iCode);
        return BAD;
    }
    
    printf("BRACKET entry order sent successfully\n");
    
    // Wait a moment for the entry order to be processed
    Sleep(100);
    
    // Second order - stop loss (stop market order)
    StopMarketOrderParams stopParams;
    stopParams.pAccount = &g_oAccount;
    stopParams.sExchange.pData = const_cast<char*>(exchange);
    stopParams.sExchange.iDataLen = (int)strlen(exchange);
    stopParams.sTicker.pData = const_cast<char*>(ticker);
    stopParams.sTicker.iDataLen = (int)strlen(ticker);
    stopParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "S" : "B");
    stopParams.sBuySellType.iDataLen = 1;
    stopParams.sDuration = sORDER_DURATION_DAY;
    stopParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    stopParams.iQty = quantity;
    stopParams.dTriggerPrice = stopPrice;
    stopParams.sTradeRoute = g_sTradeRoute;
    
    if (!g_pEngine->sendOrder(&stopParams, &iCode)) {
        printf("Error sending BRACKET stop order: %d\n", iCode);
        return BAD;
    }
    
    printf("BRACKET stop order sent successfully\n");
    
    // Wait a moment for the stop order to be processed
    Sleep(100);
    
    // Third order - take profit (limit order)
    LimitOrderParams targetParams;
    targetParams.pAccount = &g_oAccount;
    targetParams.sExchange.pData = const_cast<char*>(exchange);
    targetParams.sExchange.iDataLen = (int)strlen(exchange);
    targetParams.sTicker.pData = const_cast<char*>(ticker);
    targetParams.sTicker.iDataLen = (int)strlen(ticker);
    targetParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "S" : "B");
    targetParams.sBuySellType.iDataLen = 1;
    targetParams.sDuration = sORDER_DURATION_DAY;
    targetParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    targetParams.iQty = quantity;
    targetParams.dPrice = targetPrice;
    targetParams.sTradeRoute = g_sTradeRoute;
    
    if (!g_pEngine->sendOrder(&targetParams, &iCode)) {
        printf("Error sending BRACKET target order: %d\n", iCode);
        return BAD;
    }
    
    printf("BRACKET target order sent successfully\n");
    printf("BRACKET order complete - all three orders sent\n");
    return GOOD;
}

int sendTrailingStopOrder(const char* exchange, const char* ticker, const char* side, double stopPrice, int trailTicks, int quantity) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending TRAILING STOP order for %s %s %s: stop %.2f, trail %d ticks, qty %d\n", 
           side, exchange, ticker, stopPrice, trailTicks, quantity);
    
    // For trailing stops, we need to implement OCO (One-Cancels-Other) logic
    // This is a simplified implementation - in practice, you'd need more sophisticated trailing logic
    
    // Determine the correct buy/sell type based on the side
    const char* buySellType;
    if (strcmp(side, "B") == 0) {
        // Buy trailing stop - this is for entering a long position when price rises above stop
        buySellType = "B";
    } else if (strcmp(side, "S") == 0) {
        // Sell trailing stop - this is for entering a short position when price falls below stop
        buySellType = "S";
    } else {
        printf("Error: Invalid side '%s' for trailing stop order\n", side);
        return BAD;
    }
    
    // First, send a stop market order as the initial stop
    StopMarketOrderParams stopParams;
    stopParams.pAccount = &g_oAccount;
    stopParams.sExchange.pData = const_cast<char*>(exchange);
    stopParams.sExchange.iDataLen = (int)strlen(exchange);
    stopParams.sTicker.pData = const_cast<char*>(ticker);
    stopParams.sTicker.iDataLen = (int)strlen(ticker);
    stopParams.sBuySellType.pData = const_cast<char*>(buySellType);
    stopParams.sBuySellType.iDataLen = 1;
    stopParams.sDuration = sORDER_DURATION_DAY;
    stopParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
    stopParams.iQty = quantity;
    stopParams.dTriggerPrice = stopPrice;
    stopParams.sTradeRoute = g_sTradeRoute;
    
    int iCode;
    if (!g_pEngine->sendOrder(&stopParams, &iCode)) {
        printf("Error sending TRAILING STOP order: %d\n", iCode);
        return BAD;
    }
    
    printf("TRAILING STOP order sent successfully\n");
    printf("Note: Trailing stop logic requires real-time price monitoring and order modification\n");
    printf("This implementation sends a basic stop order. For true trailing stops, implement price monitoring.\n");
    return GOOD;
}

int sendTimeBasedOrder(const char* exchange, const char* ticker, const char* side, double price, int quantity, const char* duration) {
    if (!g_pEngine) {
        printf("Error: Engine not initialized\n");
        return BAD;
    }
    
    printf("Sending TIME-BASED order for %s %s %s at price %.2f, qty %d, duration %s\n", 
           side, exchange, ticker, price, quantity, duration);
    
    // Determine the appropriate order type based on duration
    if (strcmp(duration, "IOC") == 0 || strcmp(duration, "FOK") == 0) {
        // IOC (Immediate or Cancel) and FOK (Fill or Kill) should be market orders
        MarketOrderParams marketParams;
        marketParams.pAccount = &g_oAccount;
        marketParams.sExchange.pData = const_cast<char*>(exchange);
        marketParams.sExchange.iDataLen = (int)strlen(exchange);
        marketParams.sTicker.pData = const_cast<char*>(ticker);
        marketParams.sTicker.iDataLen = (int)strlen(ticker);
        marketParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "B" : "S");
        marketParams.sBuySellType.iDataLen = 1;
        marketParams.sDuration.pData = const_cast<char*>(duration);
        marketParams.sDuration.iDataLen = (int)strlen(duration);
        marketParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
        marketParams.iQty = quantity;
        marketParams.sTradeRoute = g_sTradeRoute;
        
        int iCode;
        if (!g_pEngine->sendOrder(&marketParams, &iCode)) {
            printf("Error sending TIME-BASED market order: %d\n", iCode);
            return BAD;
        }
        
        printf("TIME-BASED market order (%s) sent successfully\n", duration);
        return GOOD;
    } else {
        // GTC (Good Till Canceled) and other durations use limit orders
        LimitOrderParams limitParams;
        limitParams.pAccount = &g_oAccount;
        limitParams.sExchange.pData = const_cast<char*>(exchange);
        limitParams.sExchange.iDataLen = (int)strlen(exchange);
        limitParams.sTicker.pData = const_cast<char*>(ticker);
        limitParams.sTicker.iDataLen = (int)strlen(ticker);
        limitParams.sBuySellType.pData = const_cast<char*>(strcmp(side, "long") == 0 ? "B" : "S");
        limitParams.sBuySellType.iDataLen = 1;
        limitParams.sDuration.pData = const_cast<char*>(duration);
        limitParams.sDuration.iDataLen = (int)strlen(duration);
        limitParams.sEntryType = sORDER_ENTRY_TYPE_MANUAL;
        limitParams.iQty = quantity;
        limitParams.dPrice = price;
        limitParams.sTradeRoute = g_sTradeRoute;
        
        int iCode;
        if (!g_pEngine->sendOrder(&limitParams, &iCode)) {
            printf("Error sending TIME-BASED limit order: %d\n", iCode);
            return BAD;
        }
        
        printf("TIME-BASED limit order (%s) sent successfully\n", duration);
        return GOOD;
    }
}

/*   =====================================================================   */

int main(int      argc,
         char * * argv,
         char * * envp)
     {
     char * USAGE = (char *)"SampleOrder user password exchange ticker [B|S] [order_type] [price] [quantity] [additional_params...]\n";

     MyAdmCallbacks *  pAdmCallbacks;
     RCallbacks *      pCallbacks;
     REngineParams     oParams;
     LoginParams       oLoginParams;
     MarketOrderParams oMktOrdParams;
     tsNCharcb         sExchange;
     tsNCharcb         sTicker;
     char **           fake_envp;
     int               iCode;
     char*             orderType = NULL;
     double            price = 0.0;
     int               quantity = 1;
     char*             orderNum = NULL;
     char*             duration = NULL;
     double            stopPrice = 0.0;
     double            targetPrice = 0.0;
     int               trailTicks = 0;

     /*   ----------------------------------------------------------------   */

     if (argc < 6)
          {
          printf("%s", USAGE);
          printf("\nAvailable order types:\n");
          printf("  - open_long: Open long position (use 0.0 for market order, or specify price for limit order)\n");
          printf("  - open_short: Open short position (use 0.0 for market order, or specify price for limit order)\n");
          printf("  - exit_long: Exit long position (market order)\n");
          printf("  - exit_short: Exit short position (market order)\n");
          printf("  - stop_long: Place stop order for long position\n");
          printf("  - stop_short: Place stop order for short position\n");
          printf("  - modify: Modify existing order (requires order number)\n");
          printf("  - cancel: Cancel existing order (requires order number)\n");
          printf("  - bracket: Bracket order with entry/stop/target\n");
          printf("  - trailing_stop: Trailing stop order\n");
          printf("  - time_based: Time-based order (GTC, IOC, FOK)\n");
          printf("\nExamples:\n");
          printf("  SampleOrder user pass CBOT ZCU5 B open_long 0.0 1    # Market order\n");
          printf("  SampleOrder user pass CBOT ZCU5 B open_long 450.25 1  # Limit order\n");
          printf("  SampleOrder user pass CBOT ZCU5 B modify 177693517 450.50 2\n");
          printf("  SampleOrder user pass CBOT ZCU5 B cancel 177693517\n");
          printf("  SampleOrder user pass CBOT ZCU5 B bracket 450.25 440.00 460.00 1  # entry stop target qty\n");
          printf("  SampleOrder user pass CBOT ZCU5 B trailing_stop 440.00 5 1  # stop trail_ticks qty\n");
          printf("  SampleOrder user pass CBOT ZCU5 B time_based 450.25 1 GTC\n");
          printf("Note: Use full contract symbol (e.g., ZCU5 for Corn Dec 2025)\n");
          return (BAD);
          }

     /*   ----------------------------------------------------------------   */
     /*   Parse additional arguments */
     if (argc >= 7) orderType = argv[6];
     if (argc >= 8) price = atof(argv[7]);
     if (argc >= 9) quantity = atoi(argv[8]);
     
     // Parse additional parameters for specific order types
     if (orderType && strcmp(orderType, "modify") == 0) {
          if (argc >= 8) orderNum = argv[7];
          if (argc >= 9) price = atof(argv[8]);
          if (argc >= 10) quantity = atoi(argv[9]);
     } else if (orderType && strcmp(orderType, "cancel") == 0) {
          if (argc >= 8) orderNum = argv[7];
     } else if (orderType && strcmp(orderType, "bracket") == 0) {
          if (argc >= 8) price = atof(argv[7]);        // entry_price is argv[7]
          if (argc >= 9) stopPrice = atof(argv[8]);    // stop_price is argv[8]
          if (argc >= 10) targetPrice = atof(argv[9]); // target_price is argv[9]
          if (argc >= 11) quantity = atoi(argv[10]);   // quantity is argv[10]
          printf("DEBUG: Bracket order parsing - argc=%d, entry=%.2f, stop=%.2f, target=%.2f, qty=%d\n", 
                 argc, price, stopPrice, targetPrice, quantity);
     } else if (orderType && strcmp(orderType, "trailing_stop") == 0) {
          if (argc >= 8) stopPrice = atof(argv[7]);     // stop_price is argv[7]
          if (argc >= 9) trailTicks = atoi(argv[8]);    // trail_ticks is argv[8]
          if (argc >= 10) quantity = atoi(argv[9]);     // quantity is argv[9]
          printf("DEBUG: Trailing stop parsing - argc=%d, stop=%.2f, trail=%d, qty=%d\n", 
                 argc, stopPrice, trailTicks, quantity);
     } else if (orderType && strcmp(orderType, "time_based") == 0) {
          if (argc >= 10) duration = argv[9];  // duration is argv[9]
     }
     
     /*   ----------------------------------------------------------------   */
     /*   Validate ticker symbol format */
     const char* ticker = argv[4];
     if ((int)(int)strlen(ticker) < 3) {
          printf("Error: Invalid ticker symbol '%s'\n", ticker);
          printf("Use full contract symbol (e.g., ZCU5 for Corn Dec 2025)\n");
          printf("Format: [SYMBOL][MONTH][YEAR] - e.g., ZCU5, ESU5, NQU5\n");
          return (BAD);
     }
     
     // Check if ticker looks like a valid contract symbol (has month/year)
     bool hasMonthYear = false;
     if ((int)(int)strlen(ticker) >= 4) {
          char monthChar = ticker[(int)(int)strlen(ticker)-2];
          char yearChar = ticker[(int)(int)strlen(ticker)-1];
          // Valid month codes: F,G,H,J,K,M,N,Q,U,V,X,Z
          if (strchr("FGHJKMNQUVXZ", monthChar) && isdigit(yearChar)) {
               hasMonthYear = true;
          }
     }
     
     if (!hasMonthYear) {
          printf("Warning: Ticker '%s' may not be a valid contract symbol\n", ticker);
          printf("Expected format: [SYMBOL][MONTH][YEAR] - e.g., ZCU5, ESU5, NQU5\n");
          printf("Month codes: F=Jan, G=Feb, H=Mar, J=Apr, K=May, M=Jun, N=Jul, Q=Aug, U=Sep, V=Oct, X=Nov, Z=Dec\n");
          printf("Continuing anyway...\n");
     }

     /*   ----------------------------------------------------------------   */

     try
          {
          pAdmCallbacks = new MyAdmCallbacks();
          }
     catch (OmneException& oEx)
          {
          iCode = oEx.getErrorCode();
          printf("MyAdmCallbacks::MyAdmCallbacks() error : %d\n", iCode);
          return (BAD);
          }

     /*   ----------------------------------------------------------------   */
     /*   Read configuration from config.ini file                            */
     /*   ----------------------------------------------------------------   */
     
     // Try to find config.ini in multiple locations
     std::map<std::string, std::string> config;
     std::string configPaths[] = {"config.ini", "../config.ini", "../../config.ini"};
     bool configFound = false;
     
     for (const auto& path : configPaths) {
          config = readConfigFile(path);
          if (!config.empty()) {
               printf("Found config file: %s\n", path.c_str());
               configFound = true;
               break;
          }
     }
     
     if (!configFound) {
          printf("Error: Could not find config.ini file in any of the expected locations:\n");
          printf("  - ./config.ini\n");
          printf("  - ../config.ini\n");
          printf("  - ../../config.ini\n");
          printf("Please ensure config.ini exists in one of these locations.\n");
          return (BAD);
     }
     
     // Check if required config values exist
     if (config["Environment.MML_DMN_SRVR_ADDR"].empty() || 
         config["Environment.MML_DOMAIN_NAME"].empty() ||
         config["Environment.MML_LIC_SRVR_ADDR"].empty() ||
         config["Environment.MML_LOC_BROK_ADDR"].empty() ||
         config["Environment.MML_LOGGER_ADDR"].empty() ||
         config["Environment.MML_LOG_TYPE"].empty() ||
         config["Environment.MML_SSL_CLNT_AUTH_FILE"].empty() ||
         config["Environment.USER"].empty() ||
         config["Application.AppName"].empty() ||
         config["Application.AppVersion"].empty() ||
         config["Application.LogFilePath"].empty() ||
         config["Login.MdCnnctPt"].empty() ||
         config["Login.TsCnnctPt"].empty()) {
          printf("Error: Missing required configuration values in config.ini\n");
          printf("Please ensure all required sections and values are present.\n");
          return (BAD);
     }
     
     // Allocate memory for environment variables with proper buffer sizes
     char** env_vars = new char*[9];
     env_vars[0] = new char[config["Environment.MML_DMN_SRVR_ADDR"].length() + strlen("MML_DMN_SRVR_ADDR=") + 1];
     env_vars[1] = new char[config["Environment.MML_DOMAIN_NAME"].length() + strlen("MML_DOMAIN_NAME=") + 1];
     env_vars[2] = new char[config["Environment.MML_LIC_SRVR_ADDR"].length() + strlen("MML_LIC_SRVR_ADDR=") + 1];
     env_vars[3] = new char[config["Environment.MML_LOC_BROK_ADDR"].length() + strlen("MML_LOC_BROK_ADDR=") + 1];
     env_vars[4] = new char[config["Environment.MML_LOGGER_ADDR"].length() + strlen("MML_LOGGER_ADDR=") + 1];
     env_vars[5] = new char[config["Environment.MML_LOG_TYPE"].length() + strlen("MML_LOG_TYPE=") + 1];
     env_vars[6] = new char[config["Environment.MML_SSL_CLNT_AUTH_FILE"].length() + strlen("MML_SSL_CLNT_AUTH_FILE=") + 1];
     env_vars[7] = new char[config["Environment.USER"].length() + strlen("USER=") + 1];
     env_vars[8] = NULL;
     
     // Set environment variables from config with error checking
     int result;
     result = sprintf_s(env_vars[0], config["Environment.MML_DMN_SRVR_ADDR"].length() + strlen("MML_DMN_SRVR_ADDR=") + 1, "MML_DMN_SRVR_ADDR=%s", config["Environment.MML_DMN_SRVR_ADDR"].c_str());
     if (result < 0) { printf("Error formatting MML_DMN_SRVR_ADDR\n"); return (BAD); }
     
     result = sprintf_s(env_vars[1], config["Environment.MML_DOMAIN_NAME"].length() + strlen("MML_DOMAIN_NAME=") + 1, "MML_DOMAIN_NAME=%s", config["Environment.MML_DOMAIN_NAME"].c_str());
     if (result < 0) { printf("Error formatting MML_DOMAIN_NAME\n"); return (BAD); }
     
     result = sprintf_s(env_vars[2], config["Environment.MML_LIC_SRVR_ADDR"].length() + strlen("MML_LIC_SRVR_ADDR=") + 1, "MML_LIC_SRVR_ADDR=%s", config["Environment.MML_LIC_SRVR_ADDR"].c_str());
     if (result < 0) { printf("Error formatting MML_LIC_SRVR_ADDR\n"); return (BAD); }
     
     result = sprintf_s(env_vars[3], config["Environment.MML_LOC_BROK_ADDR"].length() + strlen("MML_LOC_BROK_ADDR=") + 1, "MML_LOC_BROK_ADDR=%s", config["Environment.MML_LOC_BROK_ADDR"].c_str());
     if (result < 0) { printf("Error formatting MML_LOC_BROK_ADDR\n"); return (BAD); }
     
     result = sprintf_s(env_vars[4], config["Environment.MML_LOGGER_ADDR"].length() + strlen("MML_LOGGER_ADDR=") + 1, "MML_LOGGER_ADDR=%s", config["Environment.MML_LOGGER_ADDR"].c_str());
     if (result < 0) { printf("Error formatting MML_LOGGER_ADDR\n"); return (BAD); }
     
     result = sprintf_s(env_vars[5], config["Environment.MML_LOG_TYPE"].length() + strlen("MML_LOG_TYPE=") + 1, "MML_LOG_TYPE=%s", config["Environment.MML_LOG_TYPE"].c_str());
     if (result < 0) { printf("Error formatting MML_LOG_TYPE\n"); return (BAD); }
     
     result = sprintf_s(env_vars[6], config["Environment.MML_SSL_CLNT_AUTH_FILE"].length() + strlen("MML_SSL_CLNT_AUTH_FILE=") + 1, "MML_SSL_CLNT_AUTH_FILE=%s", config["Environment.MML_SSL_CLNT_AUTH_FILE"].c_str());
     if (result < 0) { printf("Error formatting MML_SSL_CLNT_AUTH_FILE\n"); return (BAD); }
     
     result = sprintf_s(env_vars[7], config["Environment.USER"].length() + strlen("USER=") + 1, "USER=%s", config["Environment.USER"].c_str());
     if (result < 0) { printf("Error formatting USER\n"); return (BAD); }
     
     fake_envp = env_vars;
     /*   ----------------------------------------------------------------   */

     oParams.sAppName.pData        = const_cast<char*>(config["Application.AppName"].c_str());
     oParams.sAppName.iDataLen     = (int)(int)strlen(oParams.sAppName.pData);
     oParams.sAppVersion.pData     = const_cast<char*>(config["Application.AppVersion"].c_str());
     oParams.sAppVersion.iDataLen  = (int)(int)strlen(oParams.sAppVersion.pData);
     oParams.envp                  = fake_envp;
     oParams.pAdmCallbacks         = pAdmCallbacks;
     oParams.sLogFilePath.pData    = const_cast<char*>(config["Application.LogFilePath"].c_str());
     oParams.sLogFilePath.iDataLen = (int)(int)strlen(oParams.sLogFilePath.pData);

     /*   ----------------------------------------------------------------   */

     try
          {
          g_pEngine = new REngine(&oParams);
          }
     catch (OmneException& oEx)
          {
          cleanupResources(NULL, NULL, pAdmCallbacks, fake_envp);

          iCode = oEx.getErrorCode();
          printf("REngine::REngine() error : %d\n", iCode);
          return (BAD);
          }

     /*   ----------------------------------------------------------------   */
     /*   instantiate a callback object - prerequisite for logging in */
     try
          {
          pCallbacks = new MyCallbacks();
          }
     catch (OmneException& oEx)
          {
          cleanupResources(g_pEngine, NULL, pAdmCallbacks, fake_envp);

          iCode = oEx.getErrorCode();
          printf("MyCallbacks::MyCallbacks() error : %d\n", iCode);
          return (BAD);
          }

     /*   ----------------------------------------------------------------   */
     /*   Set up parameters for logging in.  Again, the MdCnnctPt and        */
     /*   TsCnnctPt have values for Rithmic 01 Test.  Add values for other   */
     /*   members of LoginParams to log into other subsystems of the         */
     /*   infrastructure like like pnl and history.                          */

     oLoginParams.pCallbacks           = pCallbacks;

     oLoginParams.sMdUser.pData        = const_cast<char*>(argv[1]);
     oLoginParams.sMdUser.iDataLen     = (int)(int)strlen(oLoginParams.sMdUser.pData);

     oLoginParams.sMdPassword.pData    = const_cast<char*>(argv[2]);
     oLoginParams.sMdPassword.iDataLen = (int)(int)strlen(oLoginParams.sMdPassword.pData);

     oLoginParams.sMdCnnctPt.pData     = const_cast<char*>(config["Login.MdCnnctPt"].c_str());
     oLoginParams.sMdCnnctPt.iDataLen  = (int)(int)strlen(oLoginParams.sMdCnnctPt.pData);

     oLoginParams.sTsUser.pData        = const_cast<char*>(argv[1]);
     oLoginParams.sTsUser.iDataLen     = (int)(int)strlen(oLoginParams.sTsUser.pData);

     oLoginParams.sTsPassword.pData    = const_cast<char*>(argv[2]);
     oLoginParams.sTsPassword.iDataLen = (int)(int)strlen(oLoginParams.sTsPassword.pData);

     oLoginParams.sTsCnnctPt.pData     = const_cast<char*>(config["Login.TsCnnctPt"].c_str());
     oLoginParams.sTsCnnctPt.iDataLen  = (int)(int)strlen(oLoginParams.sTsCnnctPt.pData);

     /*   ----------------------------------------------------------------   */

     if (!g_pEngine -> login(&oLoginParams, &iCode))
          {
          printf("REngine::login() error : %d\n", iCode);

          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);

          return (BAD);
          }

     /*   ----------------------------------------------------------------   */
     /*   After calling REngine::login, RCallbacks::Alert will be called a   */
     /*   number of times.  Wait for when the login to the TsCnnctPt is      */
     /*   complete.  (See MyCallbacks::Alert() for details).                 */

     int loginTimeout = 30; // 30 second timeout
     while (!g_bTsLoginComplete && loginTimeout > 0)
          {
#ifndef WinOS
          sleep(1);
#else
          Sleep(1000);
#endif
          loginTimeout--;
          }
          
     if (loginTimeout <= 0) {
          printf("ERROR: Login timeout - no response from server\n");
          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);
          return (BAD);
     }
     
     if (g_bLoginFailed) {
          printf("ERROR: Login failed - cannot proceed\n");
          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);
          return (BAD);
     }

     /*   ----------------------------------------------------------------   */
     /*   Once logged in, we request price increment info for the instrument */
     /*   that we want to trade.  This call will return price increment      */
     /*   information as well as set up internal instrument-specific data.   */

     sExchange.pData    = const_cast<char*>(argv[3]);
     sExchange.iDataLen = (int)(int)strlen(sExchange.pData);
     sTicker.pData      = const_cast<char*>(argv[4]);
     sTicker.iDataLen   = (int)(int)strlen(sTicker.pData);

     if (!g_pEngine -> getPriceIncrInfo(&sExchange, &sTicker, &iCode))
          {
          printf("REngine::getPriceIncrInfo() error : %d\n", iCode);

          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);

          return (BAD);
          }

     /*   ----------------------------------------------------------------   */
     /*   Use the global boolean as the signaller of when the callback has   */
     /*   been fired (and our internal instrument-specific data is ready.)   */

     int priceTimeout = 10; // 10 second timeout
     while (!g_bRcvdPriceIncr && priceTimeout > 0)
          {
#ifndef WinOS
          sleep(1);
#else
          Sleep(1000);
#endif
          priceTimeout--;
          }
          
     if (priceTimeout <= 0) {
          printf("ERROR: Price increment info timeout\n");
          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);
          return (BAD);
     }

     /*   ----------------------------------------------------------------   */
     /*   Placing an order requires an account for the order.  Wait for      */
     /*   an account to be received via RCallbacks::AccountList().           */

     int accountTimeout = 10; // 10 second timeout
     while (!g_bRcvdAccount && accountTimeout > 0)
          {
#ifndef WinOS
          sleep(1);
#else
          Sleep(1000);
#endif
          accountTimeout--;
          }
          
     if (accountTimeout <= 0) {
          printf("ERROR: Account info timeout\n");
          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);
          return (BAD);
     }

     /*   ----------------------------------------------------------------   */
     /*   Placing an order requires a trade route to be specified.  Based    */
     /*   on your FCM and/or IB, you may have zero to many trade routes      */
     /*   for a given exchange.  Store the exchange where the callback can   */
     /*   see it, and then request the list.                                 */

     memcpy(&g_cExchange, 
	    sExchange.pData,
	    sExchange.iDataLen);

     g_sExchange.pData    = g_cExchange;
     g_sExchange.iDataLen = sExchange.iDataLen;

     /*   ----------------------------------------------------------------   */

     if (!g_pEngine -> listTradeRoutes(NULL, &iCode))
	  {
          printf("REngine::listTradeRoutes() error : %d\n", iCode);

          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);

          return (BAD);
	  }

     int tradeRouteTimeout = 10; // 10 second timeout
     while (!g_bRcvdTradeRoutes && tradeRouteTimeout > 0)
          {
#ifndef WinOS
          sleep(1);
#else
          Sleep(1000);
#endif
          tradeRouteTimeout--;
          }
          
     if (tradeRouteTimeout <= 0) {
          printf("ERROR: Trade routes timeout\n");
          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);
          return (BAD);
     }

     if (g_sTradeRoute.iDataLen == 0)
	  {
	  printf("No available trade routes for : %*.*s::%*.*s::%*.*s\n",
		 g_oAccount.sFcmId.iDataLen,
		 g_oAccount.sFcmId.iDataLen,
		 g_oAccount.sFcmId.pData,
		 g_oAccount.sIbId.iDataLen,
		 g_oAccount.sIbId.iDataLen,
		 g_oAccount.sIbId.pData,
		 g_sExchange.iDataLen,
		 g_sExchange.iDataLen,
		 g_sExchange.pData);

          cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);

          return (BAD);
	  }

     /*   ----------------------------------------------------------------   */
     /*   send the order based on order type */

    int orderResult = GOOD;
    
    if (orderType && strcmp(orderType, "open_long") == 0) {
         orderResult = sendOpenLongOrder(argv[3], argv[4], price, quantity);
    } else if (orderType && strcmp(orderType, "open_short") == 0) {
         orderResult = sendOpenShortOrder(argv[3], argv[4], price, quantity);
    } else if (orderType && strcmp(orderType, "exit_long") == 0) {
         orderResult = sendRealTimeExitOrder(argv[3], argv[4], "long", price, quantity);
    } else if (orderType && strcmp(orderType, "exit_short") == 0) {
         orderResult = sendRealTimeExitOrder(argv[3], argv[4], "short", price, quantity);
    } else if (orderType && strcmp(orderType, "stop_long") == 0) {
         orderResult = sendStopOrder(argv[3], argv[4], "long", price, quantity);
    } else if (orderType && strcmp(orderType, "stop_short") == 0) {
         orderResult = sendStopOrder(argv[3], argv[4], "short", price, quantity);
    } else if (orderType && strcmp(orderType, "modify") == 0) {
         if (!orderNum) {
              printf("Error: Order number required for modify order\n");
              orderResult = BAD;
         } else {
              orderResult = sendModifyOrder(argv[3], argv[4], orderNum, price, quantity);
         }
    } else if (orderType && strcmp(orderType, "cancel") == 0) {
         if (!orderNum) {
              printf("Error: Order number required for cancel order\n");
              orderResult = BAD;
         } else {
              orderResult = sendCancelOrder(argv[3], argv[4], orderNum);
         }
    } else if (orderType && strcmp(orderType, "bracket") == 0) {
         printf("DEBUG: Calling sendBracketOrder with: exchange=%s, ticker=%s, side=%s, price=%.2f, stopPrice=%.2f, targetPrice=%.2f, quantity=%d\n", 
                argv[3], argv[4], argv[5], price, stopPrice, targetPrice, quantity);
         orderResult = sendBracketOrder(argv[3], argv[4], argv[5], price, stopPrice, targetPrice, quantity);
    } else if (orderType && strcmp(orderType, "trailing_stop") == 0) {
         orderResult = sendTrailingStopOrder(argv[3], argv[4], argv[5], stopPrice, trailTicks, quantity);
    } else if (orderType && strcmp(orderType, "time_based") == 0) {
         if (!duration) {
              printf("Error: Duration required for time-based order (GTC, IOC, FOK)\n");
              orderResult = BAD;
         } else {
              orderResult = sendTimeBasedOrder(argv[3], argv[4], argv[5], price, quantity, duration);
         }
    } else {
         // Default behavior - original market order
         oMktOrdParams.sExchange             = sExchange;
         oMktOrdParams.sTicker               = sTicker;
         oMktOrdParams.pAccount              = &g_oAccount;
         oMktOrdParams.iQty                  = quantity;
         oMktOrdParams.sBuySellType.pData    = const_cast<char*>(argv[5]);
         oMktOrdParams.sBuySellType.iDataLen = (int)(int)strlen(oMktOrdParams.sBuySellType.pData);
         oMktOrdParams.sDuration             = sORDER_DURATION_DAY;
         oMktOrdParams.sEntryType            = sORDER_ENTRY_TYPE_MANUAL;
         oMktOrdParams.sTradeRoute           = g_sTradeRoute;

         if (!g_pEngine -> sendOrder(&oMktOrdParams, &iCode)) {
              printf("REngine::sendOrder() error : %d\n", iCode);
              orderResult = BAD;
         }
    }

    if (orderResult != GOOD) {
         cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);
         return (BAD);
    }

    /*   ----------------------------------------------------------------   */
    /*   A number of Order*Report and LineInfo callbacks will be fired.     */
    /*   Wait for the order to complete (see MyCallbacks::LineUpdate()      */
    /*   for details.                                                       */

    while (!g_bDone)
         {
#ifndef WinOS
         sleep(1);
#else
         Sleep(1000);
#endif
         }

     /*   ----------------------------------------------------------------   */
     /*   The order is complete.  Clean up and exit.                         */

     cleanupResources(g_pEngine, pCallbacks, pAdmCallbacks, fake_envp);

     /*   ----------------------------------------------------------------   */

     int iUsecs = ((g_iFromExchSsboe - g_iToExchSsboe) * 1000 * 1000) +
                  (g_iFromExchUsecs - g_iToExchUsecs);
     printf("Exchange Latency: %d us\n", iUsecs);

     /*   ----------------------------------------------------------------   */

     return (GOOD);
     }

/*   =====================================================================   */

#include <iostream>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include "CustomMdSpi.h"
#include "CustomTradeSpi.h"
#include "TickToKlineHelper.h"
#include "mysql.h"
using namespace std;

// 链接库
// #pragma comment (lib, "thostmduserapi.lib")
// #pragma comment (lib, "thosttraderapi.lib")

// ---- 全局变量 ---- //
// 公共参数
TThostFtdcBrokerIDType gBrokerID = "9999";                         // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID = "154790";                         // 投资者账户名
TThostFtdcPasswordType gInvesterPassword = "123qwe";                     // 投资者密码

// 行情参数
CThostFtdcMdApi *g_pMdUserApi = nullptr;                           // 前置地址  第一组 180.168.146.187:10110  第二组 180.168.146.187:10131
char gMdFrontAddr[] = "tcp://180.168.146.187:10110";               // 模拟行情前置地址
//char *g_pInstrumentID[] = {"TF2010", "zn2010", "cs2010"}; // 行情合约代码列表，中、上、大、郑交易所各选一种
//int instrumentNum = 3;                                             // 行情合约订阅数量
char* g_pInstrumentID[] = { "rb2010"}; // 行情合约代码列表，中、上、大、郑交易所各选一种
int instrumentNum = 1;                                             // 行情合约订阅数量
unordered_map<string, TickToKlineHelper> g_KlineHash;              // 不同合约的k线存储表

// 交易参数
CThostFtdcTraderApi *g_pTradeUserApi = nullptr;                    // 交易指针
char gTradeFrontAddr[] = "tcp://180.168.146.187:10130";            // 模拟交易前置地址  第一组：180.168.146.187:10100	第二组：180.168.146.187:10130
TThostFtdcInstrumentIDType g_pTradeInstrumentID = "zn2010";        // 所交易的合约代码
TThostFtdcDirectionType gTradeDirection = THOST_FTDC_D_Sell;       // 买卖方向
TThostFtdcPriceType gLimitPrice = 22735;                           // 交易价格


/* MYSQL 参数 */
// 以下是MYSQL部分
MYSQL* con; //= mysql_init((MYSQL*) 0); 
MYSQL_RES* res;
MYSQL_ROW row;
char tmp[400];
//database configuartion
char dbuser[30] = "root";
char dbpasswd[30] = "Quattro!"; // it must be    changed
char dbip[30] = "localhost";
char dbname[50] = "paike";
char tablename[50] = "depthmarket";
char* query = NULL;

//int x;
//int y;
//unsigned int t;
int rt;  //return value  
int num = 0;

/* MSYQL END */


int main()
{

	// 账号密码
	cout << "请输入账号： ";
	scanf("%s", gInvesterID);
	cout << "请输入密码： ";
	scanf("%s", gInvesterPassword);

	//// 初始化MYSQL
	con = mysql_init((MYSQL*)0);

	if (con != NULL && mysql_real_connect(con, dbip, dbuser, dbpasswd, dbname, 3306, NULL, 0)) {
		if (!mysql_select_db(con, dbname)) {
			printf("Select successfully the database!\n");
			con->reconnect = 1;
			query = "set names \'utf8\'";
			rt = mysql_real_query(con, query, strlen(query));
			if (rt) {
				printf("Error making query: %s !!!\n", mysql_error(con));
			}
			else {
				printf("query %s succeed!\n", query);
				res = mysql_store_result(con);
				mysql_free_result(res);
			}
		}
	}
	else 
	{
		std::cout << "Unable to connect the database,check your configuration!" << std::endl;
	}

	/*
	sprintf(tmp, "INSERT INTO contract (name) VALUES ('rb2010')");
	rt = mysql_real_query(con, tmp, strlen(tmp));
	if (rt)
	{
		printf("Error making query: %s !!!\n", mysql_error(con));
	}
	else
	{
		printf("%s executed!!!\n", tmp);
		res = mysql_store_result(con);
		mysql_free_result(res);
	}
	
	
	 
	rt = mysql_real_query(con, tmp, strlen(tmp));
	if (rt)
	{
		printf("Error making query: %s !!!\n", mysql_error(con));
	}
	else
	{
		printf("%s executed!!!\n", tmp);
		res = mysql_store_result(con);
		while (row = mysql_fetch_row(res)) {
			for (t = 0; t < mysql_num_fields(res); t++) {
				printf("%s  \n\n", row[t]);
			}
			num++;
		}
		printf("number of rows %d\n", num);
		mysql_free_result(res);
	}
	*/
	

	/*
	// 初始化行情线程
	cout << "初始化行情..." << endl;
	g_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();   // 创建行情实例
	CThostFtdcMdSpi *pMdUserSpi = new CustomMdSpi;       // 创建行情回调实例
	g_pMdUserApi->RegisterSpi(pMdUserSpi);               // 注册事件类
	g_pMdUserApi->RegisterFront(gMdFrontAddr);           // 设置行情前置地址
	g_pMdUserApi->Init();                                // 连接运行
	*/
	


	// 初始化交易线程
	cout << "初始化交易..." << endl;
	g_pTradeUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(); // 创建交易实例
	CustomTradeSpi *pTradeSpi = new CustomTradeSpi;               // 创建交易回调实例
	g_pTradeUserApi->RegisterSpi(pTradeSpi);                      // 注册事件类
	g_pTradeUserApi->SubscribePublicTopic(THOST_TERT_RESTART);    // 订阅公共流
	g_pTradeUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);   // 订阅私有流
	g_pTradeUserApi->RegisterFront(gTradeFrontAddr);              // 设置交易前置地址
	g_pTradeUserApi->Init();                                      // 连接运行
		

	/*
	// 行情线程
	g_pMdUserApi->Join();
	delete pMdUserSpi;
	g_pMdUserApi->Release();
	*/



	// 交易线程
	g_pTradeUserApi->Join();
	delete pTradeSpi;
	g_pTradeUserApi->Release();


	// 释放MySQL
	mysql_close(con);

	
	getchar();
	return 0;
}
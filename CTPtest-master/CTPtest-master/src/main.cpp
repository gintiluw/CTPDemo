#include <iostream>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include "CustomMdSpi.h"
#include "CustomTradeSpi.h"
#include "TickToKlineHelper.h"
#include "mysql.h"
using namespace std;

// ���ӿ�
// #pragma comment (lib, "thostmduserapi.lib")
// #pragma comment (lib, "thosttraderapi.lib")

// ---- ȫ�ֱ��� ---- //
// ��������
TThostFtdcBrokerIDType gBrokerID = "9999";                         // ģ�⾭���̴���
TThostFtdcInvestorIDType gInvesterID = "154790";                         // Ͷ�����˻���
TThostFtdcPasswordType gInvesterPassword = "123qwe";                     // Ͷ��������

// �������
CThostFtdcMdApi *g_pMdUserApi = nullptr;                           // ǰ�õ�ַ  ��һ�� 180.168.146.187:10110  �ڶ��� 180.168.146.187:10131
char gMdFrontAddr[] = "tcp://180.168.146.187:10110";               // ģ������ǰ�õ�ַ
//char *g_pInstrumentID[] = {"TF2010", "zn2010", "cs2010"}; // �����Լ�����б��С��ϡ���֣��������ѡһ��
//int instrumentNum = 3;                                             // �����Լ��������
char* g_pInstrumentID[] = { "rb2010"}; // �����Լ�����б��С��ϡ���֣��������ѡһ��
int instrumentNum = 1;                                             // �����Լ��������
unordered_map<string, TickToKlineHelper> g_KlineHash;              // ��ͬ��Լ��k�ߴ洢��

// ���ײ���
CThostFtdcTraderApi *g_pTradeUserApi = nullptr;                    // ����ָ��
char gTradeFrontAddr[] = "tcp://180.168.146.187:10130";            // ģ�⽻��ǰ�õ�ַ  ��һ�飺180.168.146.187:10100	�ڶ��飺180.168.146.187:10130
TThostFtdcInstrumentIDType g_pTradeInstrumentID = "zn2010";        // �����׵ĺ�Լ����
TThostFtdcDirectionType gTradeDirection = THOST_FTDC_D_Sell;       // ��������
TThostFtdcPriceType gLimitPrice = 22735;                           // ���׼۸�


/* MYSQL ���� */
// ������MYSQL����
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

	// �˺�����
	cout << "�������˺ţ� ";
	scanf("%s", gInvesterID);
	cout << "���������룺 ";
	scanf("%s", gInvesterPassword);

	//// ��ʼ��MYSQL
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
	// ��ʼ�������߳�
	cout << "��ʼ������..." << endl;
	g_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();   // ��������ʵ��
	CThostFtdcMdSpi *pMdUserSpi = new CustomMdSpi;       // ��������ص�ʵ��
	g_pMdUserApi->RegisterSpi(pMdUserSpi);               // ע���¼���
	g_pMdUserApi->RegisterFront(gMdFrontAddr);           // ��������ǰ�õ�ַ
	g_pMdUserApi->Init();                                // ��������
	*/
	


	// ��ʼ�������߳�
	cout << "��ʼ������..." << endl;
	g_pTradeUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(); // ��������ʵ��
	CustomTradeSpi *pTradeSpi = new CustomTradeSpi;               // �������׻ص�ʵ��
	g_pTradeUserApi->RegisterSpi(pTradeSpi);                      // ע���¼���
	g_pTradeUserApi->SubscribePublicTopic(THOST_TERT_RESTART);    // ���Ĺ�����
	g_pTradeUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);   // ����˽����
	g_pTradeUserApi->RegisterFront(gTradeFrontAddr);              // ���ý���ǰ�õ�ַ
	g_pTradeUserApi->Init();                                      // ��������
		

	/*
	// �����߳�
	g_pMdUserApi->Join();
	delete pMdUserSpi;
	g_pMdUserApi->Release();
	*/



	// �����߳�
	g_pTradeUserApi->Join();
	delete pTradeSpi;
	g_pTradeUserApi->Release();


	// �ͷ�MySQL
	mysql_close(con);

	
	getchar();
	return 0;
}
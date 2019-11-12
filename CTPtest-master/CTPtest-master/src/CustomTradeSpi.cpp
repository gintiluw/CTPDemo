#include <iostream>
#include <time.h>
#include <thread>
#include <chrono>
#include "CustomTradeSpi.h"
#include "StrategyTrade.h"
#include "mysql.h"

// ---- ȫ�ֲ������� ---- //
extern TThostFtdcBrokerIDType gBrokerID;                      // ģ�⾭���̴���
extern TThostFtdcInvestorIDType gInvesterID;                  // Ͷ�����˻���
extern TThostFtdcPasswordType gInvesterPassword;              // Ͷ��������
extern CThostFtdcTraderApi *g_pTradeUserApi;                  // ����ָ��
extern char gTradeFrontAddr[];                                // ģ�⽻��ǰ�õ�ַ
extern TThostFtdcInstrumentIDType g_pTradeInstrumentID;       // �����׵ĺ�Լ����
extern TThostFtdcDirectionType gTradeDirection;               // ��������
extern TThostFtdcPriceType gLimitPrice;                       // ���׼۸�

// �Ự����
TThostFtdcFrontIDType	trade_front_id;	//ǰ�ñ��
TThostFtdcSessionIDType	session_id;	//�Ự���
TThostFtdcOrderRefType	order_ref;	//��������
time_t lOrderTime;
time_t lOrderOkTime;

char *InstrumentName[5000];
int Count(0);
TThostFtdcInstrumentIDType InstrumentName2[5000];
bool OnRspQryInstrument_bIsLast(false);

// mysql����
extern MYSQL* con;
extern MYSQL_RES* res;
extern MYSQL_ROW row;
extern char tmp[400];
extern char* query;
extern int rt;
extern int num;
bool OnQueryInstrument = (false);



void CustomTradeSpi::OnFrontConnected()
{
	
	std::cout << "OnFrontConnected..." << std::endl;
	static const char* version = g_pTradeUserApi->GetApiVersion();
	std::cout << "Version:" << version << std::endl;
	// ��ʼ��¼
	// reqUserLogin();

	// ��͸ʽ �ѵ�¼�ӿڻ�����Ȩ�ӿ�
	reqAuthenticate();
}


void CustomTradeSpi::OnRspAuthenticate(
	CThostFtdcRspAuthenticateField* pRspAuthenticateField, 
	CThostFtdcRspInfoField* pRspInfo, 
	int nRequestID, 
	bool bIsLast)
{
	printf("OnRspAuthenticate..\n");
	if (!isErrorRspInfo(pRspInfo))
	{
		printf("authenticate success.., ErrorID=0x%O4x, ErrMsg=%s\n\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		// ��Ȩ�Ժ��ٵ�¼
		reqUserLogin();
	}
	else
	{
		std::cout << "��֤ʧ��" << "ErrorID=" << pRspInfo->ErrorID << ", ErrMsg=" << pRspInfo->ErrorID << std::endl;
	}

}


void CustomTradeSpi::OnRspUserLogin(
	CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		printf("OnRspUserLogin....\n\n");
		loginFlag = true;
		std::cout << "�����գ� " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "��¼ʱ�䣺 " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "�����̣� " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "�ʻ����� " << pRspUserLogin->UserID << std::endl;
		// ����Ự����
		trade_front_id = pRspUserLogin->FrontID;
		session_id = pRspUserLogin->SessionID;
		strcpy(order_ref, pRspUserLogin->MaxOrderRef);

		// Ͷ���߽�����ȷ��
		reqSettlementInfoConfirm();
	}
}

void CustomTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	isErrorRspInfo(pRspInfo);
}

void CustomTradeSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "=====�������ӶϿ�=====" << std::endl;
	std::cerr << "�����룺 " << nReason << std::endl;
}

void CustomTradeSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "=====����������ʱ=====" << std::endl;
	std::cerr << "���ϴ�����ʱ�䣺 " << nTimeLapse << std::endl;
}

void CustomTradeSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField *pUserLogout,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		loginFlag = false; // �ǳ��Ͳ����ٽ����� 
		std::cout << "=====�˻��ǳ��ɹ�=====" << std::endl;
		std::cout << "�����̣� " << pUserLogout->BrokerID << std::endl;
		std::cout << "�ʻ����� " << pUserLogout->UserID << std::endl;
	}
}

void CustomTradeSpi::OnRspSettlementInfoConfirm(
	CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====Ͷ���߽�����ȷ�ϳɹ�=====" << std::endl;
		std::cout << "ȷ�����ڣ� " << pSettlementInfoConfirm->ConfirmDate << std::endl;
		std::cout << "ȷ��ʱ�䣺 " << pSettlementInfoConfirm->ConfirmTime << std::endl;
		// �����ѯ��Լ
		reqQueryInstrument();
	}
}

void CustomTradeSpi::OnRspQryInstrument(
	CThostFtdcInstrumentField *pInstrument,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	/* �޸� */
	char *temp = pInstrument->InstrumentID;
	InstrumentName[Count] = temp;                                     //���������InstrumentName��������������������Ұָ������
	memcpy(&InstrumentName2[Count], &(pInstrument->InstrumentID), sizeof(TThostFtdcInstrumentIDType));
	std::cout << InstrumentName2[Count] << std::endl;
	std::cout << pInstrument->InstrumentName << std::endl;
	
	sprintf(tmp, "select * from %s where name=\'%s\'", "contract", InstrumentName2[Count]);
	rt = mysql_real_query(con, tmp, strlen(tmp));
	if (rt)
	{
		printf("Error making query: %s !!!\n", mysql_error(con));
	}
	else
	{
		printf("%s executed!!!\n", tmp);
		res = mysql_store_result(con);
		if (mysql_num_fields(res) == 0) {
			OnQueryInstrument = true;
		}
		else {
			OnQueryInstrument = false;
		}
		mysql_free_result(res);
	}
	
	if (OnQueryInstrument) {
		sprintf(tmp, "INSERT INTO contract (name) VALUES ('%s')", InstrumentName2[Count]);
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
	}
	else {
		printf("��Լ�Ѵ���\n");
	}
	
	
	
	Count++;    
	if (bIsLast == true)
	{
		std::cout << "�ܺ�Լ��:" << Count << std::endl;
		OnRspQryInstrument_bIsLast = true;
	}
	/* �޸� */

	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====��ѯ��Լ����ɹ�=====" << std::endl;
		// std::cout << "���������룺 " << pInstrument->ExchangeID << std::endl;
		// std::cout << "��Լ���룺 " << pInstrument->InstrumentID << std::endl;
		// std::cout << "��Լ�ڽ������Ĵ��룺 " << pInstrument->ExchangeInstID << std::endl;
		// std::cout << "ִ�мۣ� " << pInstrument->StrikePrice << std::endl;
		// std::cout << "�����գ� " << pInstrument->EndDelivDate << std::endl;
		// std::cout << "��ǰ����״̬�� " << pInstrument->IsTrading << std::endl;
		// �����ѯͶ�����ʽ��˻�
		// reqQueryTradingAccount();
	}
}

void CustomTradeSpi::OnRspQryTradingAccount(
	CThostFtdcTradingAccountField *pTradingAccount,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====��ѯͶ�����ʽ��˻��ɹ�=====" << std::endl;
		std::cout << "Ͷ�����˺ţ� " << pTradingAccount->AccountID << std::endl;
		std::cout << "�����ʽ� " << pTradingAccount->Available << std::endl;
		std::cout << "��ȡ�ʽ� " << pTradingAccount->WithdrawQuota << std::endl;
		std::cout << "��ǰ��֤��: " << pTradingAccount->CurrMargin << std::endl;
		std::cout << "ƽ��ӯ���� " << pTradingAccount->CloseProfit << std::endl;
		// �����ѯͶ���ֲ߳�
		reqQueryInvestorPosition();
	}
}

void CustomTradeSpi::OnRspQryInvestorPosition(
	CThostFtdcInvestorPositionField *pInvestorPosition,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====��ѯͶ���ֲֳ߳ɹ�=====" << std::endl;
		if (pInvestorPosition)
		{
			std::cout << "��Լ���룺 " << pInvestorPosition->InstrumentID << std::endl;
			std::cout << "���ּ۸� " << pInvestorPosition->OpenAmount << std::endl;
			std::cout << "�������� " << pInvestorPosition->OpenVolume << std::endl;
			std::cout << "���ַ��� " << pInvestorPosition->PosiDirection << std::endl;
			std::cout << "ռ�ñ�֤��" << pInvestorPosition->UseMargin << std::endl;
		}
		else
			std::cout << "----->�ú�Լδ�ֲ�" << std::endl;
		
		// ����¼������������һ���ӿڣ��˴��ǰ�˳��ִ�У�
		/*if (loginFlag)
			reqOrderInsert();*/
		//if (loginFlag)
		//	reqOrderInsertWithParams(g_pTradeInstrumentID, gLimitPrice, 1, gTradeDirection); // �Զ���һ�ʽ���

		// ���Խ���
		/*
		std::cout << "=====��ʼ������Խ���=====" << std::endl;
		while (loginFlag)
			StrategyCheckAndTrade(g_pTradeInstrumentID, this);
			*/
	}
}

void CustomTradeSpi::OnRspOrderInsert(
	CThostFtdcInputOrderField *pInputOrder, 
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====����¼��ɹ�=====" << std::endl;
		std::cout << "��Լ���룺 " << pInputOrder->InstrumentID << std::endl;
		std::cout << "ʱ�䣺 " << pInputOrder->GTDDate << std::endl;
		std::cout << "�۸� " << pInputOrder->LimitPrice << std::endl;
		std::cout << "������ " << pInputOrder->VolumeTotalOriginal << std::endl;  // ����=��һ������-ǰһ������
		std::cout << "�������� " << pInputOrder->Direction << std::endl;        // 
		
	}
}

void CustomTradeSpi::OnRspOrderAction(
	CThostFtdcInputOrderActionField *pInputOrderAction,
	CThostFtdcRspInfoField *pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====���������ɹ�=====" << std::endl;
		std::cout << "��Լ���룺 " << pInputOrderAction->InstrumentID << std::endl;
		std::cout << "������־�� " << pInputOrderAction->ActionFlag;
	}
}

void CustomTradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	char str[10];
	sprintf(str, "%d", pOrder->OrderSubmitStatus);
	int orderState = atoi(str) - 48;	//����״̬0=�Ѿ��ύ��3=�Ѿ�����

	std::cout << "=====�յ�����Ӧ��=====" << std::endl;

	if (isMyOrder(pOrder))
	{
		if (isTradingOrder(pOrder))
		{
			std::cout << "--->>> �ȴ��ɽ��У�" << std::endl;
			//reqOrderAction(pOrder); // ������Գ���
			//reqUserLogout(); // �ǳ�����
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			std::cout << "--->>> �����ɹ���" << std::endl;
	}
}

void CustomTradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	std::cout << "=====�����ɹ��ɽ�=====" << std::endl;
	std::cout << "�ɽ�ʱ�䣺 " << pTrade->TradeTime << std::endl;
	std::cout << "��Լ���룺 " << pTrade->InstrumentID << std::endl;
	std::cout << "�ɽ��۸� " << pTrade->Price << std::endl;
	std::cout << "�ɽ����� " << pTrade->Volume << std::endl;
	std::cout << "��ƽ�ַ��� " << pTrade->Direction << std::endl;
}

bool CustomTradeSpi::isErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "���ش���--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
	return bResult;
}

void CustomTradeSpi::reqUserLogin()
{
	printf("ReqUserLogin....\n\n");
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, gBrokerID);
	strcpy(loginReq.UserID, gInvesterID);
	strcpy(loginReq.Password, gInvesterPassword);
	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
		std::cout << ">>>>>>���͵�¼����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���͵�¼����ʧ��" << std::endl;
}


void CustomTradeSpi::reqAuthenticate() {
	CThostFtdcReqAuthenticateField authenticateReq;
	memset(&authenticateReq, 0, sizeof(authenticateReq));
	strcpy(authenticateReq.BrokerID, gBrokerID);
	strcpy(authenticateReq.UserID, gInvesterID);
	strcpy(authenticateReq.AppID, "simnow_client_test");
	strcpy(authenticateReq.AuthCode, "0000000000000000");
	static int requestID = 0; // ������
	int iResult = g_pTradeUserApi->ReqAuthenticate(&authenticateReq, requestID);
	std::cerr << "--->>> ������֤����" << ((iResult == 0) ? "�ɹ�" : "ʧ��") << std::endl;
}


void CustomTradeSpi::reqUserLogout()
{
	CThostFtdcUserLogoutField logoutReq;
	memset(&logoutReq, 0, sizeof(logoutReq));
	strcpy(logoutReq.BrokerID, gBrokerID);
	strcpy(logoutReq.UserID, gInvesterID);
	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqUserLogout(&logoutReq, requestID);
	if (!rt)
		std::cout << ">>>>>>���͵ǳ�����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���͵ǳ�����ʧ��" << std::endl;
}


void CustomTradeSpi::reqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
	memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
	strcpy(settlementConfirmReq.BrokerID, gBrokerID);
	strcpy(settlementConfirmReq.InvestorID, gInvesterID);
	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqSettlementInfoConfirm(&settlementConfirmReq, requestID);
	if (!rt)
		std::cout << ">>>>>>����Ͷ���߽�����ȷ������ɹ�" << std::endl;
	else
		std::cerr << "--->>>����Ͷ���߽�����ȷ������ʧ��" << std::endl;
}

void CustomTradeSpi::reqQueryInstrument()
{
	CThostFtdcQryInstrumentField instrumentReq;
	memset(&instrumentReq, 0, sizeof(instrumentReq));
	// strcpy(instrumentReq.InstrumentID, g_pTradeInstrumentID);
	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqQryInstrument(&instrumentReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͺ�Լ��ѯ����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͺ�Լ��ѯ����ʧ��" << std::endl;
}

void CustomTradeSpi::reqQueryTradingAccount()
{
	CThostFtdcQryTradingAccountField tradingAccountReq;
	memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
	strcpy(tradingAccountReq.BrokerID, gBrokerID);
	strcpy(tradingAccountReq.InvestorID, gInvesterID);
	static int requestID = 0; // ������
	std::this_thread::sleep_for(std::chrono::milliseconds(700)); // ��ʱ����Ҫͣ��һ����ܲ�ѯ�ɹ�
	int rt = g_pTradeUserApi->ReqQryTradingAccount(&tradingAccountReq, requestID);
	if (!rt)
		std::cout << ">>>>>>����Ͷ�����ʽ��˻���ѯ����ɹ�" << std::endl;
	else
		std::cerr << "--->>>����Ͷ�����ʽ��˻���ѯ����ʧ��" << std::endl;
}

void CustomTradeSpi::reqQueryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField postionReq;
	memset(&postionReq, 0, sizeof(postionReq));
	strcpy(postionReq.BrokerID, gBrokerID);
	strcpy(postionReq.InvestorID, gInvesterID);
	strcpy(postionReq.InstrumentID, g_pTradeInstrumentID);
	static int requestID = 0; // ������
	std::this_thread::sleep_for(std::chrono::milliseconds(700)); // ��ʱ����Ҫͣ��һ����ܲ�ѯ�ɹ�
	int rt = g_pTradeUserApi->ReqQryInvestorPosition(&postionReq, requestID);
	if (!rt)
		std::cout << ">>>>>>����Ͷ���ֲֲ߳�ѯ����ɹ�" << std::endl;
	else
		std::cerr << "--->>>����Ͷ���ֲֲ߳�ѯ����ʧ��" << std::endl;
}

void CustomTradeSpi::reqOrderInsert()
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///���͹�˾����
	strcpy(orderInsertReq.BrokerID, gBrokerID);
	///Ͷ���ߴ���
	strcpy(orderInsertReq.InvestorID, gInvesterID);
	///��Լ����
	strcpy(orderInsertReq.InstrumentID, g_pTradeInstrumentID);
	///��������
	strcpy(orderInsertReq.OrderRef, order_ref);
	///�����۸�����: �޼�
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///��������: 
	orderInsertReq.Direction = gTradeDirection;
	///��Ͽ�ƽ��־: ����
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///���Ͷ���ױ���־
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	orderInsertReq.LimitPrice = gLimitPrice;
	///������1
	orderInsertReq.VolumeTotalOriginal = 1;
	///��Ч������: ������Ч
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///�ɽ�������: �κ�����
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	orderInsertReq.MinVolume = 1;
	///��������: ����
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ǿƽԭ��: ��ǿƽ
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	orderInsertReq.IsAutoSuspend = 0;
	///�û�ǿ����־: ��
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͱ���¼������ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͱ���¼������ʧ��" << std::endl;
}

void CustomTradeSpi::reqOrderInsert(
	TThostFtdcInstrumentIDType instrumentID,
	TThostFtdcPriceType price,
	TThostFtdcVolumeType volume,
	TThostFtdcDirectionType direction)
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///���͹�˾����
	strcpy(orderInsertReq.BrokerID, gBrokerID);
	///Ͷ���ߴ���
	strcpy(orderInsertReq.InvestorID, gInvesterID);
	///��Լ����
	strcpy(orderInsertReq.InstrumentID, instrumentID);
	///��������
	strcpy(orderInsertReq.OrderRef, order_ref);
	///�����۸�����: �޼�
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///��������: 
	orderInsertReq.Direction = direction;
	///��Ͽ�ƽ��־: ����
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///���Ͷ���ױ���־
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	orderInsertReq.LimitPrice = price;
	///������1
	orderInsertReq.VolumeTotalOriginal = volume;
	///��Ч������: ������Ч
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///�ɽ�������: �κ�����
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	orderInsertReq.MinVolume = 1;
	///��������: ����
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ǿƽԭ��: ��ǿƽ
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	orderInsertReq.IsAutoSuspend = 0;
	///�û�ǿ����־: ��
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͱ���¼������ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͱ���¼������ʧ��" << std::endl;
}

void CustomTradeSpi::reqOrderAction(CThostFtdcOrderField *pOrder)
{
	static bool orderActionSentFlag = false; // �Ƿ����˱���
	if (orderActionSentFlag)
		return;

	CThostFtdcInputOrderActionField orderActionReq;
	memset(&orderActionReq, 0, sizeof(orderActionReq));
	///���͹�˾����
	strcpy(orderActionReq.BrokerID, pOrder->BrokerID);
	///Ͷ���ߴ���
	strcpy(orderActionReq.InvestorID, pOrder->InvestorID);
	///������������
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///��������
	strcpy(orderActionReq.OrderRef, pOrder->OrderRef);
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	orderActionReq.FrontID = trade_front_id;
	///�Ự���
	orderActionReq.SessionID = session_id;
	///����������
	//	TThostFtdcExchangeIDType	ExchangeID;
	///�������
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///������־
	orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
	///�۸�
	//	TThostFtdcPriceType	LimitPrice;
	///�����仯
	//	TThostFtdcVolumeType	VolumeChange;
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(orderActionReq.InstrumentID, pOrder->InstrumentID);
	static int requestID = 0; // ������
	int rt = g_pTradeUserApi->ReqOrderAction(&orderActionReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͱ�����������ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͱ�����������ʧ��" << std::endl;
	orderActionSentFlag = true;
}

bool CustomTradeSpi::isMyOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->FrontID == trade_front_id) &&
		(pOrder->SessionID == session_id) &&
		(strcmp(pOrder->OrderRef, order_ref) == 0));
}

bool CustomTradeSpi::isTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}
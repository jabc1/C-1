
static void ap_task(PKT_T *pkt, AP_TASK_T task) {

	switch (task) {
		case AP_INIT:        	//AP��ʼ��
			ap_beacom_timer_start();
			ap_enable_int();
			break;
		case AP_SEND_BEACOM:   //AP���͹㲥��
			ap_send_beacom();
			break;
		case AP_RCVD_SYNC:     //AP�յ�ͬ��������
			//break;
		case AP_SEND_SYNC_ACK: //AP�ظ�ͬ��ACK
			ap_send_nw_info(pkg->id);
			break;
		case AP_RCVD_PULL:     //AP�յ�PULL����
			ap_reset_beacom_id(pkt->data.id);
			ap_send_to_eth(pkt->data.id);
			break;
		case AP_SEND_DATA:     //AP��������
			if (ap_wakeuped_id(pkt->data.id) == true) {
				ap_set_beacom_id(pkt->data.id);
				ap_send_data(pkt->data.data);
			} else {
				ap_set_wakeup_id(pkt->data.id);
			}
			break;
		case AP_RECV_DATA_ACK: //AP�յ�����ACK
			ap_send_to_eth(pkt);
			break;
		case AP_SLEEP:         //AP��������
		default:
			wait_for_int();		
			break;	
	}
}

void ap_main(void) {
	PKT_T eth_pkg;
	UINT32 id;
	PKT_T rf_pkg;
	RING_BUF	task_buf;
	AP_TASK_T task;
	
	ap_init_task_ring_buf(&task_buf);
	ap_register_eth_int(&eth_pkg);
	ap_register_rf_int(&rf_pkg);
	ap_start_beacom_timer();
	
	ap_task(AP_INIT);

	while (1) {
		task = get_task(&task_buf);
		ap_task(task != AP_SEND_DATA ? &rf_pkg : &eth_pkg, task);
	}
}

static void ep_task(PKT *pkt) {
	switch (ep_task) {
		case EP_INIT:          //�ն˳�ʼ��
			break;
		case EP_RECV_BEACOM:   //�ն˽��չ㲥��
			break;
		case EP_SEND_SYNC:     //�ն˷���ͬ������
			break;
		case EP_RECV_SYNC_ACK: //�ն��ܵ�ͬ��ACK
			break;
		case EP_SEND_PULL:     //�ն˷���PULL������
			break;
		case EP_RECV_DATA:     //�ն��յ�����
			break;
		case EP_SEND_DATA_ACK: //�ն˻ظ�����ACK
			break;
		case EP_SLEEP:          //�ն˽�������	
			break;
		default:
			break;	
	}
}









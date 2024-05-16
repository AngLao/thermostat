#include "EasyPact.h"

uint8_t REC_HEADER = 0xAA;
uint8_t check_model = sum_model;
uint8_t transfer_model = high_first_out;
/**
 * @brief ����֡����
 * @param new_data �µ�����
 * @param frame �洢�������֡����
 * @return �����ɹ����� 0�����򷵻� 1
 */
uint8_t easy_parse_data(frame_t** frame,const uint8_t new_data)
{
    static parse_state_t	    parse_state = HEADER;
    static uint8_t 			 	data_index  = 0;
    static frame_t 			 	current_frame ;  // ��̬�����洢��ǰ֡��״̬������
    static uint8_t* 			p_frame = (uint8_t*)&current_frame ;  // һ�����ڲ���֡���ݵ�ָ��,ʹ��uint8_t������λ����

    uint8_t parse_fail  = 1;

    switch (parse_state) {
    case HEADER: {
        // �ȴ�֡ͷ�ĵ�һ���ֽ�
        if (new_data == REC_HEADER) {
            *(p_frame+HEADER) = new_data;
            parse_state = ADDRESS;
        }
        break;
    }
    case ADDRESS: {
        // �ȴ���ַ�ֽ�
        *(p_frame+ADDRESS) = new_data;
        parse_state = ID;
        break;
    }
    case ID: {
        // �ȴ� ID �ֽ�
        *(p_frame+ID) = new_data;
        parse_state = DATA_LENGTH;
        break;
    }
    case DATA_LENGTH: {
        // �ȴ����ݳ����ֽ�
        *(p_frame+DATA_LENGTH) = new_data;
        if (*(p_frame+DATA_LENGTH) > MAX_DATA_LENGTH) {
            //���ݳ��ȳ�����Χ�����ý���״̬
            parse_state = HEADER;
        } else {
            //������������
            data_index = 0;
            //û��Ҫ���յ�����ֱ�ӽ��ռ�����
            parse_state = (*(p_frame+DATA_LENGTH) > 0) ? DATA : CHECK_HIGH;
        }
        break;
    }
    case DATA: {
        // �ȴ������ֽ�
        *(p_frame+DATA+data_index) = new_data;
        data_index++;
        //�������ݳ��ȷ��ϴ������ݳ���
        if (data_index == *(p_frame+DATA_LENGTH)) {
            // ���ݽ�����ϣ�������һ��״̬
            parse_state = CHECK_HIGH;
        }
        break;
    }
    case CHECK_HIGH: {
        // �ȴ�У�����λ�ֽ� ���ݺ��һλ��ΪУ������ֽ�
        *(p_frame+DATA+data_index) = new_data  ;

        parse_state = CHECK_LOW;
        break;
    }
    case CHECK_LOW: {
        // �ȴ�У�����λ�ֽ� ���ݺ�ڶ�λ��ΪУ������ֽ�
        *(p_frame+DATA+data_index+1) = new_data;
        uint16_t checksum = calculate_check(&current_frame,*(p_frame+DATA_LENGTH)+4);
        uint16_t this_checksum = ((uint16_t)*(p_frame+DATA+data_index)<<8)+*(p_frame+DATA+data_index+1);
        if (this_checksum == checksum){
            // У����ƥ�䣬�����ɹ�
            *frame =  &current_frame;
            parse_fail = 0;
        }
        parse_state = HEADER;
        break;
    }
    }

    return parse_fail;
}

/**
 * @brief  ��֡�н�����֡����
 * @param  p_Frame ָ��֡�ṹ���ָ��
 * @return uint16_t ֡����
 */
uint16_t easy_return_name(frame_t* p_Frame)
{
    // ��֡�ж�ȡID�͵�ַ�����֡����
    uint16_t name =p_Frame->address << 8  ;
    name |=p_Frame->id  ;
    return name;
}

/**
 * @brief  ��֡�н�����֡����
 * @param  p_Frame ָ��֡�ṹ���ָ��
 * @return uint8_t ֡�ܳ���
 */
uint8_t easy_return_buflen(frame_t *p_Frame)
{

    uint8_t* p = (uint8_t*)p_Frame;
    return (*(p+DATA_LENGTH)+6) ;
}

/**
 * @brief ������֡�����ָ�����ȵ�����
 * @param p_Frame ����ָ֡��
 * @param data Ҫ��ӵ�����
 * @param length Ҫ��ӵ����ݳ��ȣ���λΪ�ֽ�
 * @return ������ݵĽ����0��ʾ�ɹ���1��ʾ���ݳ��ȳ����������
 */
uint8_t easy_add_data(frame_t *p_Frame, uint32_t data, uint8_t length)
{
    uint8_t* p = (uint8_t*)p_Frame;
    uint8_t old_length = *(p+DATA_LENGTH);
    //���ݳ��ȳ����������
    if((old_length+length) >MAX_DATA_LENGTH )
        return 1;
    //������ʵ���ݳ���
    *(p+DATA_LENGTH) += length;
    //�����ʵ����
    for(uint8_t i =0; i<length; i++){
        if(transfer_model == high_first_out){
            switch (length) {
            case sizeof(int):
                *(p+DATA+old_length+i) = data>>(8*(3-i));
                break;
            case sizeof(short):
                *(p+DATA+old_length+i) = data>>(8*(1-i));
                break;
            case sizeof(char):
                *(p+DATA+old_length+i) = data>>(8*i);
                break;
            }
        }else{
            *(p+DATA+old_length+i) = data>>(8*i);
        }
    }
    return 0;
}

/**
 * @brief ���ڶ�ָ������֡����У�飬����У������ӵ�����֡��β����
 * @param p_Frame ָ���У�������ָ֡�롣
 * @return У������Ϊ0��ʾУ��ɹ���
 */
uint8_t easy_add_check(frame_t *p_Frame)
{
    uint8_t* p = (uint8_t*)p_Frame;    // ��ָ������ת��Ϊ uint8_t ָ������
    uint8_t frame_len = *(p+DATA_LENGTH) + 4 ;    // ��������֡�ܳ���
    uint16_t check = calculate_check(p_Frame , frame_len);    // ������֡����У��

    /*ָ�뼴Ϊ0*/
    *(p+frame_len) = (check>>8);       // ��У������λ�洢������֡β��
    *(p+frame_len+1)  = check;         // ��У������λ�洢������֡β��

    return 0;    // ����У������Ϊ0��ʾУ��ɹ�
}


/**
 * @brief ��������ָ������֡�� HEADER �ֶ�ֵ��
 * @param p_Frame ָ������� HEADER ������ָ֡�롣
 * @param header ָ���� HEADER ֵ��
 */
void easy_set_header(frame_t *p_Frame, uint8_t header)
{
    uint8_t* p = (uint8_t*)p_Frame;    // ��ָ������ת��Ϊ uint8_t ָ������
    *(p+HEADER) = header;              // �� HEADER �ֶ�����Ϊָ���� HEADER ֵ
}

/**
 * @brief ��������ָ������֡�� ADDRESS �ֶ�ֵ��
 * @param p_Frame ָ������� ADDRESS ������ָ֡�롣
 * @param address ָ���� ADDRESS ֵ��
 */
void easy_set_address(frame_t *p_Frame, uint8_t address)
{
    uint8_t* p = (uint8_t*)p_Frame;    // ��ָ������ת��Ϊ uint8_t ָ������
    *(p+ADDRESS) = address;            // �� ADDRESS �ֶ�����Ϊָ���� ADDRESS ֵ
}

/**
 * @brief ��������ָ������֡�� ID �ֶ�ֵ��
 * @param p_Frame ָ������� ID ������ָ֡�롣
 * @param id ָ���� ID ֵ��
 */
void easy_set_id(frame_t *p_Frame, uint8_t id)
{
    uint8_t* p = (uint8_t*)p_Frame;  // ��ָ������ת��Ϊ uint8_t ָ������
    *(p+ID) = id;                    // �� ID �ֶ�����Ϊָ���� ID ֵ
}

/**
 * @brief ���ڽ�ָ������֡�����ݲ��ֵ������ֽڶ����㡣
 * @param p_Frame ָ���������ݵ�����ָ֡�롣
 */
void easy_wipe_data(frame_t *p_Frame)
{
    uint8_t* p = (uint8_t*)p_Frame;
    *(p+DATA_LENGTH) = 0;
}


/**
 * @brief ����check_modelѡ����Ӧ��У���㷨����У��
 * @param p_Frame ��У�������ָ֡��
 * @param length ��У������ݳ���
 * @return У����������check_model��ͬ����ΪУ��ͻ�CRCУ����
*/
uint16_t calculate_check(frame_t *p_Frame, uint8_t length)
{
    uint16_t res = 0;
    if(check_model == sum_model) // ���ʹ��У���ģʽ
        res = calculate_sum(p_Frame,length);
    else if(check_model == crc16_model) // ���ʹ��CRC16ģʽ
        res = calculate_crc(p_Frame,length);
    return res;
}

/**
 * @brief ��������֡��У���
 * @param p_Frame ��У�������ָ֡��
 * @param length ��У������ݳ���
 * @return У���
*/
uint16_t calculate_sum(frame_t *p_Frame, uint8_t length)
{
    uint8_t sum = 0;
    // ����У���
    for (int i = 0; i < length; i++) {
        sum += *((uint8_t*)p_Frame+i);
    }
    return sum;
}

/**
 * @brief ����֡��CRCУ����
 * @param p_Frame ָ��֡�ṹ���ָ��
 * @param length ֡���ȣ�������У���룩
 * @return uint16_t CRCУ����
 */
uint16_t calculate_crc(frame_t *p_Frame, uint8_t length) {
#define POLY 0x1021
    uint16_t crc = 0xFFFF;

    // ��������֡������CRCУ����
    for (int i = 0; i < length ; i++) {
        crc ^= ((uint16_t)*((uint8_t*)p_Frame+i) << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

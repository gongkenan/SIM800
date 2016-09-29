// ������� ��������� 74HC165 � ������������ ������ � ���������������� ������� ���������� ��������
#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "REG74HC165.h"

struct reg74hc165_current_state reg74hc165_current_state_num1; // ����������� �������� ����� ���� ���������

// ����������: ������� - ��� ������
//             ��� ������� - ��� ���������� �������, ������� �������� ����� 1 �������
void meander_recognition(struct reg74hc165_current_state * current_state)
{
	uint8_t i;
	for (i = 0; i < NUM_OF_INPUT; i++)
	{
		if ((current_state->arr_res[i].pulse_duration > NOICE_DURATION)
		    && (current_state->arr_res[i].pulse_duration < THRESHOLD_DURATION)
		    && !current_state->arr_res[i].status.cur_log_state)
		{
			current_state->arr_res[i].pulse_duration = 0;
			current_state->arr_res[i].status.is_meander = 1;
		}
	}
}

// ����������: ������� - ��� ���������� ������
//             ��� ������� - ��� ���������� �������, ������� �������� ����� 1 �������
void const_sig_recognition(struct reg74hc165_current_state * current_state)
{
	uint8_t i;
	for (i = 0; i < NUM_OF_INPUT; i++)
	{
		if ((current_state->arr_res[i].pulse_duration > NOICE_DURATION)
		    && (current_state->arr_res[i].pulse_duration > THRESHOLD_DURATION))
		{
			current_state->arr_res[i].pulse_duration = 0;
			current_state->arr_res[i].status.is_const_sig = 1;
		}
	}
}

// ����������: ���������� �������
//             ��� ������� - ��� ���������� �������, ������� �������� ����� 1 �������
void vanishing_recognition(struct reg74hc165_current_state * current_state)
{
	uint8_t i;
	for (i = 0; i < NUM_OF_INPUT; i++)
	{
		if ((current_state->arr_res[i].pause_duration > THRESHOLD_DURATION))
		{
			current_state->arr_res[i].pause_duration = 0;
			current_state->arr_res[i].status.const_already_sent = 0;
			current_state->arr_res[i].status.meandr_already_sent = 0;
			current_state->arr_res[i].status.is_const_sig = 0;
			current_state->arr_res[i].status.is_meander = 1;
		}
	}
}

// ������� ���������� �� ����������� ���������� �������
// ����������� ���������� ��������� ��������
// ������������ ���������� ��������� (�������� �������� �������� ��� ����������)
void pulse_processing(struct reg74hc165_current_state * current_state)
{
	uint8_t i;
	for (i = 0; i < NUM_OF_INPUT; i++)
	{
		if (current_state->arr_res[i].status.cur_log_state && (!current_state->arr_res[i].status.is_const_sig))
		{
			current_state->arr_res[i].pulse_duration ++;
		}

		if (!current_state->arr_res[i].status.cur_log_state &&
			(current_state->arr_res[i].status.const_already_sent || current_state->arr_res[i].status.meandr_already_sent))
		{
			current_state->arr_res[i].pause_duration ++;
		}
	}
	meander_recognition(current_state);
	const_sig_recognition(current_state);
	vanishing_recognition(current_state);
}

// ������� ���������� �� ����������� ���������� ������� ������������ �������� ������ ������ � ������ 74hc165
void load_data74HC165(struct reg74hc165_current_state * current_state)
{
	//PL_PIN_UP;
	if (current_state->stage == pl_low)
	{
		PL_PIN_DOWN;
		CP_PIN_UP;
		current_state->stage = cp_low;
		return;
	}

	if (current_state->stage == cp_low)
	{
		PL_PIN_UP;
		CP_PIN_UP;
		current_state->stage = cp_high;
		return;
	}

	if (current_state->stage == cp_high)
	{
		PL_PIN_UP;
		CP_PIN_DOWN;
		current_state->stage = qh_ready;
		return;
	}

	if (current_state->stage == qh_ready)
	{
		PL_PIN_UP;
		CP_PIN_DOWN;

		if (QH_PIN_STATE) // ���������� ������� ���
		{
			current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.cur_phis_state = 1;
		}
		else
		{
			current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.cur_phis_state = 0;
		}

		// ������������� ���������� ��������� � ����������� �� ����������� � ��� ������� ��������
		if (current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.alarm_state)
		{
			current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.cur_log_state =
					current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.cur_phis_state;
		}
		else
		{
			current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.cur_log_state =
					~current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.cur_phis_state;
		}

		current_state->current_bit++;

		if (current_state->current_bit == (NUM_OF_INPUT))
		{
			current_state->stage = pl_low;
			current_state->current_bit = 0;
			return;
		}

		current_state->stage = cp_low;
		return;
	}

	return;
}

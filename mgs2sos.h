#pragma once

#include <Windows.h>

#pragma pack(push, 1)

struct FVECTOR {
    float vec[4];
};

struct SVECTOR {
    WORD vec[4];
};

struct CAMERA_MGS2_PC {
    FVECTOR position;
    FVECTOR target;
    float angle;
    DWORD unk0;
    DWORD priority;
    DWORD enabled;
    DWORD next;
    void* interpolate_func_in;
    void* interpolate_func_out;
    WORD interpolate_time_in;
    WORD interpolate_time_out;
    FVECTOR bound1;
    FVECTOR bound2;
    FVECTOR limit1;
    FVECTOR limit2;
    SVECTOR rotate;
    DWORD track;
    DWORD camera_id;
    DWORD flag;
    DWORD channel_id;
    DWORD interpolate_id;
    BYTE unk6[52];
};

struct CAMERA_OBJECT {
    BYTE object_header[0x30];
    CAMERA_MGS2_PC camera;
};

struct CAMERA_CHANNEL_MGS2_PC {
    CAMERA_MGS2_PC cameras[9];
    CAMERA_MGS2_PC current;
    CAMERA_MGS2_PC master;
    CAMERA_MGS2_PC* next;
    void* interpolate_func;
    DWORD interpolate_time;
    DWORD flag;
    BYTE unk3[16];
};

struct DAEMON_OBJECT {
    BYTE object_header[0x30];
    CAMERA_CHANNEL_MGS2_PC channels[2];
};

struct NEW_TPS_CAMERA {
    DWORD flag1;
    float xyz_mul_preset;
    float angle_preset1;
    float z_limit1;
    float z_limit2;
    float vec_mul_preset;
    float trg_cor_preset;
    float pos_cor_preset1;
    float pos_cor_preset2;
    float angle_preset2;
    float rx;
    float ry;
    DWORD rflags1;
    DWORD rflags2;
    DWORD flag2;
    DWORD flags;
    float distance;
    WORD direction;
    float direction_delta;
    float z_value;
    DWORD z_index;
    float unk_0x328;
    float unk_0x32C;
    float trg_mul_value1;
    float pos_mul_value1;
    float trg_mul_value2;
    float pos_mul_value2;

    FVECTOR vec_pos_new_temp;
    FVECTOR vec_trg_new_temp;

    FVECTOR vec_obj_pos;

    // Pos and Trg for next camera
    FVECTOR vec_pos_new;
    FVECTOR vec_trg_new;
};

#pragma pack(pop)

extern float Sensitivity;
extern bool Invert_X;
extern bool Invert_Y;
extern int mouseMovement_X;
extern int mouseMovement_Y;

extern void camera_handler(CAMERA_OBJECT* cw);
extern int check_if_snake_is_next_to_wall_controls(BYTE* buttons);
extern void convert_stick_to_button(void* arg0, void* arg1, void* arg2, int arg3);
extern int check_if_snake_needs_to_jump(BYTE* a1);
extern void camera_transitions_handler(DAEMON_OBJECT* dw);
extern int start_paddemo(int char_id);
extern void paddemo_handler(BYTE* a1);
extern int diff_direction_for_swimming(int a1, int a2);
extern int pick_direction_for_swimming(int a1, int a2, int a3);
extern int chara_func_0xDFADB();
extern int get_camera_direction_fix_for_hanging();
extern int diff_direction_for_hanging1(int a1, int a2);
extern int diff_direction_for_hanging2(int a1, int a2);
extern void set_direction_for_PL_SubjectMove_wrap_with_set_stick_direction(void* a1);
extern int is_rightstick_used(void* a1);
extern WORD get_blade_direction(void* a1);
extern int check_if_special_blade_attack();
extern int diff_direction_for_blade_protection(int a1, int a2);
extern int raiden_event_msg_motion(void* a1, void* a2);
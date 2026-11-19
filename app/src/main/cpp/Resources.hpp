//
// Created by Administrator on 2026/2/3.
//

#ifndef SUPERCELL_RESOURCES_H
#define SUPERCELL_RESOURCES_H

#endif //SUPERCELL_RESOURCES_H

#include "opencv2/core.hpp"

namespace resources {
    // 路径
    namespace paths {
        inline std::string ARMAMENTS_FREE_RP;
//        inline std::string ARMAMENTS_TEAM_RP;
        inline std::string HOME_DETECT_RP;
        inline std::string WORD_DETECT_RP;
        inline std::string GENE_JOIN_RP;
        inline std::string GOLD_JOIN_RP;
        inline std::string GOLD_JOIN_THEN80_RP;
//        inline std::string GOLD_JOIN_SCORPION_RP;
        inline std::string DIG_TIP_RP;
        inline std::string DIG_DRONES_RP;
        inline std::string DIG_DONE_RP;
        inline std::string DIG_TREASURE_RP;
        inline std::string GOLD_TARGET_RP;
//        inline std::string GOLD_TARGET_RP_REL;
        inline std::string DIG_AWARD_DRONES_RP;
        inline std::string DIG_AWARD_TREASURE_RP;
        inline std::string DIG_GIFT_BOX_RP;
        inline std::string FINED_RP;
        inline std::string EGG_TIP_RP;
        inline std::string CANCEL_FLAG_RP;
        inline std::string POST_TIP_RP;
        inline std::string EGG_PACKAGE_RP;
        inline std::string GOLD_FILTER_70_RP;
        inline std::string GOLD_FILTER_75_RP;
        inline std::string GOLD_FILTER_THEN1_RP;
        inline std::string GOLD_FILTER_THEN4_RP;
        //clash & boom template
        inline std::string attack_axe_rp;
        inline std::string win_stars_rp;
        inline std::string arrow_switch_rp;
        inline std::string carve_next_rp;
        inline std::string car_holy_rp;

        inline std::string cookie_rescue_rp;
        inline std::string sword_attack_rp;
        inline std::string map_win_stars_rp;


        inline std::string daily_event_rp;
        inline std::string sonar_event_rp;


        inline std::string mask_axe_rp;
        inline std::string mask_stars_rp;
        inline std::string mask_switch_rp;
        inline std::string mask_holy_rp;

        inline std::string mask_cookie_rp;
        inline std::string mask_sword_rp;
        inline std::string mask_map_stars_rp;

        inline std::string mask_daily_event_rp;


    }

    // 图像
    namespace mats {
        inline cv::Mat m_GT;
        inline cv::Mat m_GT_REL;
        inline cv::Mat m_GJ;
        inline cv::Mat m_GJ_G;
        inline cv::Mat m_GJ_T80;
        inline cv::Mat m_GJ_s;
        inline cv::Mat m_GF_70;
        inline cv::Mat m_GF_75;
        inline cv::Mat m_GF_THEN1;
        inline cv::Mat m_GF_THEN4;
        inline cv::Mat m_AF;
        inline cv::Mat m_AT;
        inline cv::Mat m_HD;
        inline cv::Mat m_WD;
        inline cv::Mat m_DP;
        inline cv::Mat m_DD;
        inline cv::Mat m_DT;
        inline cv::Mat m_DA_D;
        inline cv::Mat m_DA_T;
        inline cv::Mat m_GB;
        inline cv::Mat m_CF;
        inline cv::Mat m_DO;
        inline cv::Mat m_ET;
        inline cv::Mat m_EP;
        inline cv::Mat m_PT;
        inline cv::Mat m_GF_THEN_SLOT;


        inline cv::Mat t_attack_axe;
        inline cv::Mat t_arrow_switch;
        inline cv::Mat t_win_stars;
        inline cv::Mat t_carve_next;
        inline cv::Mat t_car_holy;

        inline cv::Mat t_daily_event;
        inline cv::Mat t_sonar_event;


        inline cv::Mat m_attack_axe;
        inline cv::Mat m_arrow_switch;
        inline cv::Mat m_win_stars;
        inline cv::Mat m_car_holy;

        inline cv::Mat m_daily_event;


    }
    namespace points {
        inline cv::Point Digg(900, 1725);
        inline cv::Point PrepDig(900, 1270);
        inline cv::Point Magnifier(100, 2250);
        inline cv::Point Search(900, 2780);
        inline cv::Point TeamUp(900, 2130);
        inline cv::Point Dummy(85, 415);
        inline cv::Point TownBase(1570, 2740);
        inline cv::Point OpenEgg(900, 1875);
        inline cv::Point Back(120, 2715);
        inline cv::Point Dispatch(900, 2180);


        inline const cv::Point OVER_ATTACK(200, 1350);
        inline const cv::Point OVER_ATTACK_ENTER(1725, 1125);
        inline const cv::Point DEPLOY_BASE(2400, 420);
        inline const cv::Point DEPLOY_BASES[] = {{2400, 420},
                                                 {0,    900},
                                                 {2888, 900}};


        inline const cv::Point DIAM_SPEED(1070, 1370);
        inline const cv::Point DIAM_SPENT(1410, 1170);


        inline const cv::Point COLLECT_BTN(2190, 1520);

        inline cv::Point COOKIE_BTN(1800, 1160);
        inline cv::Point MAP_OVER_ATTACK(200, 1420);

        inline cv::Point SWIPE_SONAR_BEGIN(2350, 1605);
        inline cv::Point SWIPE_SONAR_END(2350, 605);


    }
    namespace regions {
        inline cv::Rect CORP_CANCEL_LIMIT(1400, 920, 100, 95);
        inline cv::Rect CORP_GJ_LIMIT(736, 400, 384, 2120);
        inline cv::Rect CORP_GF_LV(0, 0, 95, 65);
        inline cv::Rect CORP_GF_THEN_N(0, 0, 200, 200);
//        inline cv::Rect CORP_GT_FULL(1570, 1720, 190, 150);
        inline cv::Rect CORP_GT(1685, 1725, 75, 65);
        inline cv::Rect CORP_HW(1520, 2630, 220, 220);
        inline cv::Rect CORP_DP(1175, 2405, 220, 225);
        inline cv::Rect CORP_TD_LIMIT(0, 525, 900, 2005);
//        inline cv::Rect CORP_DA = cv::Rect(800, 1170, 200, 200);
        inline cv::Rect CORP_DA(830, 1200, 140, 140);
        inline cv::Rect CORP_GB_LIMIT(785, 1165, 230, 210);
        inline cv::Rect CORP_AF(45, 670, 105, 105);
//        inline cv::Rect CORP_AT(690, 2110, 425, 180);
        inline cv::Rect CORP_EGG_LIMIT(325, 525, 270, 2005);

        //clash & boom rect & point
        inline cv::Rect corp_attack(90, 1535, 170, 155);
        inline cv::Rect corp_switch_limit(300, 1690, 2300, 70);
//       inline  cv::Rect corp_holy_limit(0, 0, 2880, 900);
        inline cv::Rect begin_attack_sample(2300, 1165, 10, 10);
        inline cv::Rect back_home_sample(1485, 1520, 5, 5);
        inline cv::Rect car_holy_limit(1000, 0, 1315, 900);

//        beach & sonar

        inline cv::Rect daily_event_btn(2710, 580, 140, 120);
        inline cv::Rect sonar_event_btn(1930, 1540, 340, 80);
        inline cv::Rect retry_network_sample(1440, 900, 5, 5);
        inline cv::Rect red_bomb_sample(2650, 1575, 5, 5);
    }
    namespace hsv {
        inline cv::Scalar confirm_lower(40, 176, 203);
        inline cv::Scalar confirm_upper(50, 255, 255);
        inline cv::Scalar retry_network_lower(90, 30, 15);
        inline cv::Scalar retry_network_upper(102, 60, 45);
        inline cv::Scalar red_bomb_lower(142, 70, 80);
        inline cv::Scalar red_bomb_upper(162, 105, 115);

        inline cv::Scalar sonar_table_lower(193,181,160);
        inline cv::Scalar sonar_table_upper(215,206,190);
    }
    namespace binary {
        inline std::string events;
        inline std::string performance_click;
        inline std::string zoom_events;
    }

    void init(JNIEnv *env, const std::string &_user_);
}

//java.lang.NullPointerException: Attempt to read from field 'int com.example.supercell.FrameBufferInfo.rowStride' on a null object reference in method 'void com.example.supercell.MainActivity.lambda$onCreate$0()'
//at com.example.supercell.MainActivity.lambda$onCreate$0(MainActivity.java:284)

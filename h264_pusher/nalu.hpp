/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-04 16:42:26
 * @LastEditors: li
 * @LastEditTime: 2021-03-05 09:20:11
 */
#ifndef _C_NALU_HPP_
#define _C_NALU_HPP_

#include <string>
#include <vector>
using namespace std;
namespace rtsptool {

inline const uint8_t* find_next_nal(const uint8_t* start, const uint8_t* end)
{
   const uint8_t *p = start;
   /* Simply lookup "0x00000001" pattern */
   while (p <= end ) {
      if (p[0] == 0x00) {
         if (p[1] == 0x00) {
            if(p[2] == 0x00 && p[3] == 0x01) {
				p+=4;
            	return p;
            } else {
               p += 1;
            }
         } else {
            p += 2;
         }
      } else {
         ++p;
      }
   }
   return end;
}

inline void get_sps_pps_nalu(uint8_t *data, int len, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps)
{
    const uint8_t* pkt_bgn = data;
    const uint8_t* pkt_end = pkt_bgn + len;
    const uint8_t* pbgn = pkt_bgn;
    const uint8_t* pend = pkt_end;
    while (pbgn < pkt_end) {
        pbgn = find_next_nal(pbgn, pkt_end);
        if (pbgn == pkt_end) {
           continue;
        }
        pend = find_next_nal(pbgn, pkt_end);
 
        if (((*pbgn) & 0x1F) == 0x07) {    //SPS NAL
            std::cout<<"find sps nal"<<std::endl;
            sps.assign(pbgn, pbgn + static_cast<int>(pend - 4 - pbgn));
        }
        if (((*pbgn) & 0x1F) == 0x08) {    //PPS NAL
            std::cout<<"find pps nal"<<std::endl;
            if(pend==pkt_end){
               pps.assign(pbgn, pbgn + static_cast<int>(pend - pbgn));
            }else{
               pps.assign(pbgn, pbgn + static_cast<int>(pend - 4 - pbgn));
            }  
       }
       pbgn = pend - 4;
    }
}

} // namespace websocketpp

#endif // _C_NALU_HPP_

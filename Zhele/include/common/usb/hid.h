/**
 * @file
 * Implement USB-HID class
 * 
 * @author Aleksei Zhelonkin
 * @date 2021
 * @license FreeBSD
 */

#ifndef ZHELE_USB_HID_H
#define ZHELE_USB_HID_H

#include "interface.h"

#include "../template_utils/type_list.h"

#include <string.h>

namespace Zhele::Usb
{
    /**
     * @brief Implements HID report.
     * 
     * @tparam _Data Raw report bytes.
     */
    template<uint8_t... _Data>
    struct HidReport
    {
        static constexpr uint8_t Data[sizeof...(_Data)] = {_Data...};
    };

    /**
     * @brief Implements HID descriptor.
     * 
     * @tparam _Version Version.
     * @tparam _Reports All reports.
     */
    template<uint16_t _Version = 0x0200, typename... _Reports>
    struct HidImpl
    {
        uint8_t Length = 0x06 + 3 *  sizeof...(_Reports);
        uint8_t DescriptorType = 0x21;
        uint16_t Version = _Version;
        uint8_t CountryCode = 0x00;
        uint8_t ReportsCount = sizeof...(_Reports);

        /**
         * @brief Total reports size
         * 
         * @returns Reports size
         */
        static constexpr uint16_t ReportsSize()
        {
            return (static_cast<uint16_t>(sizeof(_Reports::Data)) + ...);
        }

        /**
         * @brief Fills reports descriptors
         * 
         * @param [out] address Destination memory
         * 
         * @returns Total bytes writter
         */
        static uint16_t FillReportsDescriptors(uint8_t* address)
        {
            ((*address++ = 0x22,
            *address++ = static_cast<uint8_t>(static_cast<uint16_t>(sizeof(_Reports::Data)) & 0xff),
            *address++ = static_cast<uint8_t>((static_cast<uint16_t>(sizeof(_Reports::Data)) >> 8) & 0xff)),...);

            return 3 * sizeof...(_Reports);
        }

        /**
         * @brief Fill HID reports
         * 
         * @param [out] address Destination memory
         * 
         * @returns Total bytes writter
         */
        static uint16_t FillReports(uint8_t* address)
        {
            ((memcpy(address, _Reports::Data, sizeof(_Reports::Data)), address += sizeof(_Reports::Data)), ...);

            return ReportsSize();
        }
    };

    /**
     * @brief HID interface
     * 
     * 
    template <uint8_t _Number, uint8_t _AlternateSetting, uint8_t _SubClass, uint8_t _Protocol, typename _HidImpl, typename _Ep0, typename... _Endpoints>
    class HidInterface : public Interface<_Number, _AlternateSetting, InterfaceClass::Hid, _SubClass, _Protocol, _Ep0, _Endpoints...>
    {
        using Base = Interface<_Number, _AlternateSetting, InterfaceClass::Hid, _SubClass, _Protocol, _Ep0, _Endpoints...>;
    public:
        using Endpoints = Base::Endpoints;

        static void SetupHandler()
        {
            SetupPacket* setup = reinterpret_cast<SetupPacket*>(_Ep0::RxBuffer);
            
             if (setup->Request == StandartRequestCode::GetDescriptor
                && static_cast<GetDescriptorParameter>(setup->Value) == GetDescriptorParameter::HidReportDescriptor)
            {
                uint8_t temp[HidInterface::ReportsSize()];
                uint16_t size = HidInterface::FillReports(temp);
                _Ep0::SendData(temp, setup->Length < size ? setup->Length : size);
            }
        }

        static uint16_t FillDescriptor(InterfaceDescriptor* descriptor)
        {
            uint16_t totalLength = sizeof(InterfaceDescriptor);

            *descriptor = InterfaceDescriptor {
                .Number = _Number,
                .AlternateSetting = _AlternateSetting,
                .EndpointsCount = Base::EndpointsCount,
                .Class = InterfaceClass::Hid,
                .SubClass = _SubClass,
                .Protocol = _Protocol
            };
            _HidImpl* hidDescriptor = reinterpret_cast<_HidImpl*>(++descriptor);
            *hidDescriptor = _HidImpl {
            };
            
            uint8_t* reportsPart = reinterpret_cast<uint8_t*>(++hidDescriptor);
            uint16_t bytesWritten = _HidImpl::FillReportsDescriptors(reportsPart);

            totalLength += sizeof(_HidImpl) + bytesWritten;

            EndpointDescriptor* endpointsDescriptors = reinterpret_cast<EndpointDescriptor*>(&reportsPart[bytesWritten]);
            totalLength += (0 + ... + _Endpoints::FillDescriptor(endpointsDescriptors++));

            return totalLength;
        }

        static constexpr uint16_t ReportsSize()
        {
            return _HidImpl::ReportsSize();
        }

        static uint16_t FillReports(uint8_t* destination)
        {
            return _HidImpl::FillReports(destination);
        }
    private:
    };

    template<typename T>
    class IsHidPredicate
    {
    public:
        static const bool value = std::is_base_of_v<HidTag, T>;
    };
}
#endif // ZHELE_USB_HID_H
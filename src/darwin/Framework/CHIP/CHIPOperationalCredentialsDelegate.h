/**
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#import "CHIPError_Internal.h"
#import "CHIPPersistentStorageDelegateBridge.h"

#include <controller/OperationalCredentialsDelegate.h>

NS_ASSUME_NONNULL_BEGIN

class CHIPOperationalCredentialsDelegate : public chip::Controller::OperationalCredentialsDelegate {
public:
    CHIPOperationalCredentialsDelegate() {}

    ~CHIPOperationalCredentialsDelegate() {}

    CHIP_ERROR init(CHIPPersistentStorageDelegateBridge * storage);

    CHIP_ERROR GenerateNOCChain(const chip::ByteSpan & csrElements, const chip::ByteSpan & attestationSignature,
        const chip::ByteSpan & DAC, const chip::ByteSpan & PAI, const chip::ByteSpan & PAA,
        chip::Callback::Callback<chip::Controller::OnNOCChainGeneration> * onCompletion) override;

    void SetNodeIdForNextNOCRequest(chip::NodeId nodeId) override
    {
        mNextRequestedNodeId = nodeId;
        mNodeIdRequested = true;
    }

    void SetFabricIdForNextNOCRequest(chip::FabricId fabricId) override { mNextFabricId = fabricId; }

    void SetDeviceID(chip::NodeId deviceId) { mDeviceBeingPaired = deviceId; }
    void ResetDeviceID() { mDeviceBeingPaired = chip::kUndefinedNodeId; }

private:
    CHIP_ERROR GenerateKeys();
    CHIP_ERROR StoreKeysInKeyChain(NSData * keypairData);
    CHIP_ERROR LoadKeysFromKeyChain();
    CHIP_ERROR LoadDeprecatedKeysFromKeyChain();
    CHIP_ERROR DeleteKeys();

    CHIP_ERROR ConvertToP256Keypair(NSData * keypairData);

    CHIP_ERROR SetIssuerID(CHIPPersistentStorageDelegateBridge * storage);

    bool ToChipEpochTime(uint32_t offset, uint32_t & epoch);

    chip::Crypto::P256Keypair mIssuerKey;
    uint32_t mIssuerId = 1234;

    const uint32_t kCertificateValiditySecs = 365 * 24 * 60 * 60;
    const NSString * kCHIPCADeprecatedKeyLabel = @"chip.nodeopcerts.CA:0";
    const NSData * kCHIPCADeprecatedKeyTag = [@"com.zigbee.chip.commissioner.ca.issuer.id" dataUsingEncoding:NSUTF8StringEncoding];
    const NSString * kCHIPCAKeyLabel = @"matter.nodeopcerts.CA:0";
    const NSData * kCHIPCAKeyTag = [@"com.zigbee.matter.commissioner.ca.issuer.id" dataUsingEncoding:NSUTF8StringEncoding];

    id mKeyType = (id) kSecAttrKeyTypeECSECPrimeRandom;
    id mKeySize = @256;

    CHIPPersistentStorageDelegateBridge * mStorage;

    chip::NodeId mDeviceBeingPaired = chip::kUndefinedNodeId;

    chip::NodeId mNextRequestedNodeId = 1;
    chip::FabricId mNextFabricId = 0;
    bool mNodeIdRequested = false;
};

NS_ASSUME_NONNULL_END

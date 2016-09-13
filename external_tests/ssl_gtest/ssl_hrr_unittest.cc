/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "secerr.h"
#include "ssl.h"
#include "sslerr.h"
#include "sslproto.h"

#include "gtest_utils.h"
#include "scoped_ptrs.h"
#include "tls_connect.h"
#include "tls_filter.h"
#include "tls_parser.h"

namespace nss_test {

TEST_P(TlsConnectTls13, HelloRetryRequestAbortsZeroRtt) {
  const char* k0RttData = "Such is life";
  const PRInt32 k0RttDataLen = static_cast<PRInt32>(strlen(k0RttData));

  SetupForZeroRtt();  // initial handshake as normal

  const SSLNamedGroup groups[2] = {ssl_grp_ec_secp384r1, ssl_grp_ec_secp521r1};
  server_->ConfigNamedGroups(groups, PR_ARRAY_SIZE(groups));
  client_->Set0RttEnabled(true);
  server_->Set0RttEnabled(true);
  ExpectResumption(RESUME_TICKET);

  // Send first ClientHello and send 0-RTT data
  auto capture_early_data = new TlsExtensionCapture(ssl_tls13_early_data_xtn);
  client_->SetPacketFilter(capture_early_data);
  client_->Handshake();
  EXPECT_EQ(k0RttDataLen, PR_Write(client_->ssl_fd(), k0RttData,
                                   k0RttDataLen));  // 0-RTT write.
  EXPECT_LT(0U, capture_early_data->extension().len());

  // Send the HelloRetryRequest
  auto hrr_capture =
      new TlsInspectorRecordHandshakeMessage(kTlsHandshakeHelloRetryRequest);
  server_->SetPacketFilter(hrr_capture);
  server_->Handshake();
  EXPECT_LT(0U, hrr_capture->buffer().len());

  // The server can't read
  std::vector<uint8_t> buf(k0RttDataLen);
  EXPECT_EQ(SECFailure, PR_Read(server_->ssl_fd(), buf.data(), k0RttDataLen));
  EXPECT_EQ(PR_WOULD_BLOCK_ERROR, PORT_GetError());

  // Make a new capture for the early data.
  capture_early_data = new TlsExtensionCapture(ssl_tls13_early_data_xtn);
  client_->SetPacketFilter(capture_early_data);

  // Complete the handshake successfully
  Handshake();
  ExpectEarlyDataAccepted(false);  // The server should reject 0-RTT
  CheckConnected();
  SendReceive();
  EXPECT_EQ(0U, capture_early_data->extension().len());
}

class KeyShareReplayer : public TlsExtensionFilter {
 public:
  KeyShareReplayer() {}

  virtual PacketFilter::Action FilterExtension(uint16_t extension_type,
                                               const DataBuffer& input,
                                               DataBuffer* output) {
    if (extension_type != ssl_tls13_key_share_xtn) {
      return KEEP;
    }

    if (!data_.len()) {
      data_ = input;
      return KEEP;
    }

    *output = data_;
    return CHANGE;
  }

 private:
  DataBuffer data_;
};

// This forces a HelloRetryRequest by disabling P-256 on the server.  However,
// the second ClientHello is modified so that it omits the requested share.  The
// server should reject this.
TEST_P(TlsConnectTls13, RetryWithSameKeyShare) {
  EnsureTlsSetup();
  client_->SetPacketFilter(new KeyShareReplayer());
  const SSLNamedGroup groups[2] = {ssl_grp_ec_secp384r1, ssl_grp_ec_secp521r1};
  server_->ConfigNamedGroups(groups, PR_ARRAY_SIZE(groups));
  ConnectExpectFail();
  EXPECT_EQ(SSL_ERROR_BAD_2ND_CLIENT_HELLO, server_->error_code());
  EXPECT_EQ(SSL_ERROR_ILLEGAL_PARAMETER_ALERT, client_->error_code());
}

// This tests that the second attempt at sending a ClientHello (after receiving
// a HelloRetryRequest) is correctly retransmitted.
TEST_F(TlsConnectDatagram13, DropClientSecondFlightWithHelloRetry) {
  const SSLNamedGroup groups[2] = {ssl_grp_ec_secp384r1, ssl_grp_ec_secp521r1};
  server_->ConfigNamedGroups(groups, PR_ARRAY_SIZE(groups));
  server_->SetPacketFilter(new SelectiveDropFilter(0x2));
  Connect();
}

TEST_F(TlsConnectTest, Select12AfterHelloRetryRequest) {
  EnsureTlsSetup();
  client_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_2,
                           SSL_LIBRARY_VERSION_TLS_1_3);
  server_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_2,
                           SSL_LIBRARY_VERSION_TLS_1_3);
  const SSLNamedGroup groups[2] = {ssl_grp_ec_secp384r1, ssl_grp_ec_secp521r1};
  server_->ConfigNamedGroups(groups, PR_ARRAY_SIZE(groups));
  client_->StartConnect();
  server_->StartConnect();

  client_->Handshake();
  server_->Handshake();

  // Here we replace the TLS server with one that does TLS 1.2 only.
  // This will happily send the client a TLS 1.2 ServerHello.
  TlsAgent* replacement_server =
      new TlsAgent(server_->name(), TlsAgent::SERVER, mode_);
  delete server_;
  server_ = replacement_server;
  server_->Init();
  client_->SetPeer(server_);
  server_->SetPeer(client_);
  server_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_2,
                           SSL_LIBRARY_VERSION_TLS_1_2);

  ConnectExpectFail();
  EXPECT_EQ(SSL_ERROR_ILLEGAL_PARAMETER_ALERT, server_->error_code());
  EXPECT_EQ(SSL_ERROR_RX_MALFORMED_SERVER_HELLO, client_->error_code());
}

class HelloRetryRequestAgentTest : public TlsAgentTestClient {
 protected:
  void MakeHelloRetryRequestRecord(SSLNamedGroup group, DataBuffer* hrr_record,
                                   uint32_t seq_num = 0) const {
    const uint8_t canned_hrr[] = {
        SSL_LIBRARY_VERSION_TLS_1_3 >> 8,
        SSL_LIBRARY_VERSION_TLS_1_3 & 0xff,
        0,
        0,  // The cipher suite is ignored.
        static_cast<uint8_t>(group >> 8),
        static_cast<uint8_t>(group),
        0,
        0  // no extensions
    };
    DataBuffer hrr;
    MakeHandshakeMessage(kTlsHandshakeHelloRetryRequest, canned_hrr,
                         sizeof(canned_hrr), &hrr, seq_num);
    MakeRecord(kTlsHandshakeType, SSL_LIBRARY_VERSION_TLS_1_3, hrr.data(),
               hrr.len(), hrr_record, seq_num);
  }
};

// Send two HelloRetryRequest messages in response to the ClientHello.  The are
// constructed to appear legitimate by asking for a new share in each, so that
// the client has to count to work out that the server is being unreasonable.
TEST_P(HelloRetryRequestAgentTest, SendSecondHelloRetryRequest) {
  EnsureInit();
  agent_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_3,
                          SSL_LIBRARY_VERSION_TLS_1_3);
  agent_->StartConnect();

  DataBuffer hrr_record;
  MakeHelloRetryRequestRecord(ssl_grp_ec_secp384r1, &hrr_record, 0);
  ProcessMessage(hrr_record, TlsAgent::STATE_CONNECTING);
  MakeHelloRetryRequestRecord(ssl_grp_ec_secp521r1, &hrr_record, 1);
  ProcessMessage(hrr_record, TlsAgent::STATE_ERROR,
                 SSL_ERROR_RX_UNEXPECTED_HELLO_RETRY_REQUEST);
}

// Here the client receives a HelloRetryRequest with a group that they already
// provided a share for.
TEST_P(HelloRetryRequestAgentTest, HandleBogusHelloRetryRequest) {
  EnsureInit();
  agent_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_3,
                          SSL_LIBRARY_VERSION_TLS_1_3);
  agent_->StartConnect();

  DataBuffer hrr_record;
  MakeHelloRetryRequestRecord(ssl_grp_ec_secp256r1, &hrr_record);
  ProcessMessage(hrr_record, TlsAgent::STATE_ERROR,
                 SSL_ERROR_RX_MALFORMED_HELLO_RETRY_REQUEST);
}

INSTANTIATE_TEST_CASE_P(HelloRetryRequestAgentTests, HelloRetryRequestAgentTest,
                        TlsConnectTestBase::kTlsModesAll);

}  // namespace nss_test

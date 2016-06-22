int _powerOff()
{
    bool ok = false;
    bool continue_cancel = false;
    if (_init && _pwr) {
        
        if (_cancel_all_operations) {
            continue_cancel = true;
           _cancel_all_operations = false; // make sure we can use the AT parser
        }
        for (int i=0; i<3; i++) { // try 3 times
            Serial1.println("AT+CPWROFF\r\n");
            int ret = waitFinalResp();
            if (RESP_OK == ret) {
                _pwr = false;
                // todo - add if these are automatically done on power down
                //_activated = false;
                //_attached = false;
                ok = true;
                break;
            }
            else if (RESP_ABORTED == ret) {
              
            }
            else {
                
            }
        }
    }
    digitalWrite(PWR_UC, INPUT);
    HAL_Pin_Mode(RESET_UC, INPUT);

    return ok;
}

---------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool pdp(char apn)
{
    bool ok = true;
    // bool is3G = _dev.dev == DEV_SARA_U260 || _dev.dev == DEV_SARA_U270;

    if (_init && _pwr) {

// todo - refactor
// This is setting up an external PDP context, join() creates an internal one
// which is ultimately the one that's used by the system. So no need for this.

        

       
        Serial1.println("AT+CGDCONT=1,\"IP\",\"%c\"\r\n");
        if (RESP_OK != waitFinalResp())
            goto failure;



        Serial1.println("AT+CGACT=1,1\r\n");
        if (RESP_OK != waitFinalResp()) {
            Serial1.println("AT+CEER\r\n");
            waitFinalResp();

            Serial1.println("AT+CGPADDR=1\r\n");
            if (RESP_OK != waitFinalResp())

            Serial1.println("AT+CGDCONT?\r\n");
            // +CGPADDR: 1, "99.88.111.88"
            if (RESP_OK != waitFinalResp()){}
            }

        Serial1.println("AT+CGPADDR=1\r\n");
        if (RESP_OK != waitFinalResp())
            goto failure;

        Serial1.println("AT+CGDCONT?\r\n");
        // +CGPADDR: 1, "99.88.111.88"
        if (RESP_OK != waitFinalResp())
            goto failure;


        _activated = true; // PDP

        return ok;
    }
    failure:
    return false;
}

-----------------------------------------------------------------------------------------------------------------------------------------
uint32_t join(const char* apn /*= NULL*/, const char* username /*= NULL*/,
                              const char* password /*= NULL*/ /*= AUTH_DETECT*/)
{

    if (_init && _pwr) {
        _ip = NOIP;
        int a = 0;
        bool force = false; // If we are already connected, don't force a reconnect.

        // perform GPRS attach
        Serial1.println("AT+CGATT=1\r\n");
        if (RESP_OK != waitFinalResp())
            goto failure;

        // Check the if the PSD profile is activated (a=1)
        Serial1.println("AT+UPSND=" PROFILE ",8\r\n");
        if (RESP_OK != waitFinalResp())
            goto failure;
        if (a == 1) {
            _activated = true; // PDP activated
            if (force) {
                // deactivate the PSD profile if it is already activated
                Serial1.println("AT+UPSDA=" PROFILE ",4\r\n");
                if (RESP_OK != waitFinalResp())
                    goto failure;
                a = 0;
            }
        }
        if (a == 0) {
            bool ok = false;
            _activated = false; // PDP deactived
            // try to lookup the apn settings from our local database by mccmnc
            const char* config = NULL;
                

            // Set up the dynamic IP address assignment.
            Serial1.println("AT+UPSD=" PROFILE ",7,\"0.0.0.0\"\r\n");
            if (RESP_OK != waitFinalResp())
                goto failure;

            do {

                // Set up the APN
                if (apn && *apn) {
                   Serial1.println("AT+UPSD=" PROFILE ",1,\"%s\"\r\n");
                    if (RESP_OK != waitFinalResp())
                        goto failure;
                }
                if (username && *username) {
                    Serial1.println("AT+UPSD=" PROFILE ",2,\"%s\"\r\n");
                    if (RESP_OK != waitFinalResp())
                        goto failure;
                }
                if (password && *password) {
                    Serial1.println("AT+UPSD=" PROFILE ",3,\"%s\"\r\n");
                    if (RESP_OK != waitFinalResp())
                        goto failure;
                }
                // try different Authentication Protocols
                // 0 = none
                // 1 = PAP (Password Authentication Protocol)
                // 2 = CHAP (Challenge Handshake Authentication Protocol)
                /*for (int i = AUTH_NONE; i <= AUTH_CHAP && !ok; i ++) {
                    if ((auth == AUTH_DETECT) || (auth == i)) {
                        // Set up the Authentication Protocol
                        Serial1.println("AT+UPSD=" PROFILE ",6,%d\r\n", i);
                        if (RESP_OK != waitFinalResp())
                            goto failure;
                        // Activate the PSD profile and make connection
                        Serial1.println("AT+UPSDA=" PROFILE ",3\r\n");
                        if (RESP_OK == waitFinalResp()) {
                            _activated = true; // PDP activated
                            ok = true;
                        }
                    }
                }*/
            } while (!ok && config && *config); // maybe use next setting ?
            if (!ok) {
                goto failure;
            }
        }
        //Get local IP address
        Serial1.println("AT+UPSND=" PROFILE ",0\r\n");
        if (RESP_OK != waitFinalResp())
            goto failure;

        
        _attached = true;  // GPRS
        return _ip;
    }
failure:
   
    return NOIP;
}
-------------------------------------------------------------------------------------------------------------------------------------------------
bool reconnect()
{
    bool ok = false;
    if (_activated) {
        if (!_attached) {
            /* Activates the PDP context assoc. with this profile */
            /* If GPRS is detached, this will force a re-attach */
            Serial1.println("AT+UPSDA=" PROFILE ",3\r\n");
            if (RESP_OK == waitFinalResp()) {

                //Get local IP address
                Serial1.println("AT+UPSND=" PROFILE ",0\r\n");
                if (RESP_OK == waitFinalResp()) {
                    ok = true;
                    _attached = true;
                }
            }
        }
    }
   
    return ok;
}
----------------------------------------------------------------------------------------------------------------------------------------------
bool disconnect()
{
    bool ok = false;
    bool continue_cancel = false;
    
    if (_attached) {
        if (_cancel_all_operations) {
            continue_cancel = true;
             _cancel_all_operations = false; // make sure we can use the AT parser
        }

        if (_ip != NOIP) {
            /* Deactivates the PDP context assoc. with this profile
             * ensuring that no additional data is sent or received
             * by the device. */
            Serial1.println("AT+UPSDA=" PROFILE ",4\r\n");
            if (RESP_OK == waitFinalResp()) {
                _ip = NOIP;
                ok = true;
                _attached = false;
            }
        }
    }
    if (continue_cancel) _cancel_all_operations = true;
    return ok;
}
--------------------------------------------------------------------------------------------------------------------------------------------------
bool detach(void)
{
    bool ok = false;
    bool continue_cancel = false;
    
    if (_activated) {
        if (_cancel_all_operations) {
            continue_cancel = true;
           _cancel_all_operations = false; // make sure we can use the AT parser
        }
        // if (_ip != NOIP) {  // if we disconnect() first we won't have an IP
            /* Detach from the GPRS network and conserve network resources. */
            /* Any active PDP context will also be deactivated. */
            Serial.println("AT+CGATT=0\r\n");
            if (RESP_OK != waitFinalResp()) {
                ok = true;
                _activated = false;
            }
        // }
    }
    if (continue_cancel)  _cancel_all_operations = true;
    return ok;
}
------------------------------------------------------------------------------------------------------------------------------------------------------
int _socketCloseHandleIfOpen(int socket_handle) {
    bool ok = false;

    // Check if socket_handle is open
    // AT+USOCTL=0,1
    // +USOCTL: 0,1,0
    int handle = MDM_SOCKET_ERROR;
    Serial1.println("AT+USOCTL=%d,1\r\n");
    if ((RESP_OK == waitFinalResp()) && (handle != MDM_SOCKET_ERROR)) {
        // Close it if it's open
        // AT+USOCL=0
        // OK
        Serial1.println("AT+USOCL=%d\r\n");
        if (RESP_OK == waitFinalResp()) {
            ok = true;
        }
    }

    return ok;
}
------------------------------------------------------------------------------------------------------------------------------------------------------------
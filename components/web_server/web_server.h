#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initializes and starts the web server.
     */
    esp_err_t start_web_server(void);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H

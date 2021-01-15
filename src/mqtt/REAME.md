Remember to comment this part in `HTTPClient` library or the firmware will crash:

```c++
if(_canReuse && headerName.equalsIgnoreCase("Connection")) {
    if(headerValue.indexOf("close") >= 0 && headerValue.indexOf("keep-alive") < 0) {
        _canReuse = false;
    }
}
```

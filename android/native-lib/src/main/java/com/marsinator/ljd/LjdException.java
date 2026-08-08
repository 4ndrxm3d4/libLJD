package com.marsinator.ljd;

public final class LjdException extends Exception {
    private final int code;

    public LjdException(String message, int code) {
        super(message);
        this.code = code;
    }

    /**
     * Native error code.
     */
    public int getCode() {
        return code;
    }

    @Override
    public String toString() {
        return "LjdException{code=" + code + ", message=" + getMessage() + "}";
    }
}

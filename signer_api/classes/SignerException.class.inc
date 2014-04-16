<?php

class SignerException extends Exception {
  private $systemMessage;

  public function __construct($message, $systemMessage = '', $code = 0, Exception $previous = null) {
    parent::__construct($message, $code, $previous);

    if (!empty($systemMessage)) {
      $this->systemMessage = $systemMessage;
    }
  }

  public function getSystemMessage() {
    return $this->systemMessage;
  }

}

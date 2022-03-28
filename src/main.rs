use std::sync::Arc;
use std::thread::sleep;
use std::time::Duration;

use anyhow::Result;

use esp_idf_svc::netif::EspNetifStack;
use esp_idf_svc::nvs::EspDefaultNvs;
use esp_idf_svc::sysloop::EspSysLoopStack;

use esp_idf_hal::prelude::*;

mod app_net;
mod app_prov;
mod emb_test;
mod led_esp32_c3_13;

fn main() -> Result<()> {
    esp_idf_sys::link_patches();

    emb_test::sanity_test();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    let peripherals = Peripherals::take().unwrap();

    log::info!("Init board led");
    let mut led = led_esp32_c3_13::Led::new(
        peripherals.ledc.channel0,
        peripherals.pins.gpio3.into_output()?,
        peripherals.ledc.channel1,
        peripherals.pins.gpio4.into_output()?,
        peripherals.ledc.channel2,
        peripherals.pins.gpio5.into_output()?,
        peripherals.ledc.channel3,
        peripherals.pins.gpio18.into_output()?,
        peripherals.ledc.channel4,
        peripherals.pins.gpio19.into_output()?,
        peripherals.ledc.timer0,
    )?;

    log::info!("Configured board led");

    log::debug!("LED state updated (red)");
    led.set_color(0x10, 0x00, 0x00)?;

    // init storage
    let default_nvs = Arc::new(EspDefaultNvs::new()?);

    // init network stack
    let netif_stack = Arc::new(EspNetifStack::new()?);
    let sys_loop_stack = Arc::new(EspSysLoopStack::new()?);

    // if required provisioning data is missing, go into provisioning mode
    log::info!("Checking whether wifi has been provisioned...");
    let (ssid, psk) = if let Some((ssid, psk)) = app_prov::validate_prov(default_nvs.clone())? {
        (ssid, psk)
    } else {
        log::info!("Checking whether wifi has been provisioned...");
        log::debug!("LED state updated (yellow)");
        led.set_color(0x10, 0x10, 0x00)?;
        app_prov::app_prov(
            netif_stack.clone(),
            sys_loop_stack.clone(),
            default_nvs.clone(),
        )?
        .expect("TODO!")
    };

    log::info!("LED state updated (blue)");
    led.set_color(0x10, 0x10, 0x00)?;
    app_net::app_net(netif_stack, sys_loop_stack, default_nvs, ssid, psk)?;

    log::info!("LED state updated (green)");
    led.set_color(0x00, 0x10, 0x00)?;
    sleep(Duration::from_secs(5));

    Ok(())
}

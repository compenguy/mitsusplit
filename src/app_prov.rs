use std::collections::HashSet;
use std::sync::{Arc, RwLock};
use std::{thread, time::Duration};

use anyhow::{bail, Result};

use log::info;

use embedded_svc::httpd::registry::Registry;
use embedded_svc::httpd::Response;
use embedded_svc::storage::Storage;
use embedded_svc::wifi::{
    AccessPointConfiguration, AccessPointInfo, ApIpStatus, ApStatus, AuthMethod,
    ClientConfiguration, Configuration, TransitionalState, Wifi,
};

use esp_idf_svc::httpd as idf;
use esp_idf_svc::netif::EspNetifStack;
use esp_idf_svc::nvs::EspDefaultNvs;
use esp_idf_svc::nvs_storage::EspNvsStorage;
use esp_idf_svc::sysloop::EspSysLoopStack;
use esp_idf_svc::wifi::EspWifi;

#[derive(Hash, Eq, PartialEq, Debug)]
pub(crate) struct WifiNetwork {
    ssid: String,
    rss: u8,
}

impl From<&AccessPointInfo> for WifiNetwork {
    fn from(ap_info: &AccessPointInfo) -> Self {
        Self {
            ssid: ap_info.ssid.clone(),
            rss: ap_info.signal_strength,
        }
    }
}

pub(crate) fn validate_prov(default_nvs: Arc<EspDefaultNvs>) -> Result<Option<(String, String)>> {
    let storage = EspNvsStorage::new_default(default_nvs, "provisioning", true)?;
    let ssid_opt: Option<String> = storage
        .get_raw("ssid")?
        .map(|v| String::from_utf8_lossy(&v).to_string());
    let psk_opt: Option<String> = storage
        .get_raw("psk")?
        .map(|v| String::from_utf8_lossy(&v).to_string());
    Ok(ssid_opt.zip(psk_opt))
}

pub(crate) fn app_prov(
    netif_stack: Arc<EspNetifStack>,
    sys_loop_stack: Arc<EspSysLoopStack>,
    default_nvs: Arc<EspDefaultNvs>,
) -> Result<Option<(String, String)>> {
    let mut wifi = init_wifi(netif_stack, sys_loop_stack, default_nvs.clone())?;

    info!("Wifi created, starting provisioning service...");

    let ap_set: HashSet<WifiNetwork> = HashSet::with_capacity(10);
    let ap_infos = Arc::new(RwLock::new(ap_set));

    let httpd = init_httpd(ap_infos.clone())?;
    info!("provisioning httpd service started.");

    let now = std::time::Instant::now();
    info!("starting wifi scanning background service...");
    while now.elapsed().as_secs() < 100 {
        info!(
            "background scanning has been running for {:?}",
            now.elapsed()
        );
        update_scan(&mut wifi, ap_infos.clone())?;
        thread::sleep(Duration::from_secs(1));
    }
    info!("Provisioning timeout.");

    drop(httpd);
    info!("Httpd stopped");

    {
        drop(wifi);
        info!("Wifi stopped");
    }

    validate_prov(default_nvs)
}

fn update_scan(wifi: &mut Box<EspWifi>, ap_infos: Arc<RwLock<HashSet<WifiNetwork>>>) -> Result<()> {
    info!("Updating access point list ({:?})", ap_infos);
    let mut ap_infos_writable = ap_infos
        .write()
        .map_err(|e| anyhow::anyhow!("Lock was poisoned: {}", e.to_string()))?;
    info!("Acquired write lock to wifi networks list");
    let scan_results = match wifi.scan() {
        Ok(results) => results,
        Err(e) => {
            info!("Error scanning for networks: {:?}", e);
            Vec::new()
        }
    };
    info!("Wifi network scan completed.");
    (*ap_infos_writable).extend(scan_results.iter().map(WifiNetwork::from));
    info!("Updated list of discovered wifi networks.");
    Ok(())
}

fn init_httpd(ap_infos: Arc<RwLock<HashSet<WifiNetwork>>>) -> Result<idf::Server> {
    info!("Defining provisioning httpd service...");
    let server = idf::ServerRegistry::new()
        .at("/")
        .get(|_| Ok("Hello from Rust!".into()))?
        .at("/foo")
        .get(|_| bail!("Boo, something happened!"))?
        .at("/bar")
        .get(|_| {
            Response::new(403)
                .status_message("No permissions")
                .body("You have no permissions to access this page".into())
                .into()
        })?
        .at("/panic")
        .get(|_| panic!("User requested a panic!"))?;

    info!("Starting provisioning httpd service...");
    server.start(&Default::default())
}

fn init_wifi(
    netif_stack: Arc<EspNetifStack>,
    sys_loop_stack: Arc<EspSysLoopStack>,
    default_nvs: Arc<EspDefaultNvs>,
) -> Result<Box<EspWifi>> {
    let mut wifi = Box::new(EspWifi::new(netif_stack, sys_loop_stack, default_nvs)?);

    // TODO: set PSK to something we can query the serial bus for that the user can read off the
    // unit?
    wifi.set_configuration(&Configuration::Mixed(
        ClientConfiguration {
            ssid: "mitsusplit".into(),
            password: "mitsubishi".into(),
            ..Default::default()
        },
        AccessPointConfiguration {
            ssid: "mitsusplit".into(),
            auth_method: AuthMethod::WPA2Personal,
            password: "mitsubishi".into(),
            ..Default::default()
        },
    ))?;

    info!("Wifi configuration set, about to get status");

    wifi.wait_status(|status| !status.1.is_transitional());

    let status = wifi.get_status();

    if let ApStatus::Started(ApIpStatus::Done) = status.1 {
        info!("Wifi connected");
    } else {
        bail!("Unexpected Wifi status: {:?}", status);
    }

    Ok(wifi)
}
